#include <kernix/sched.h>
#include <kernix/cpu.h>
#include <kernix/heap.h>
#include <kernix/kernel.h>
#include <kernix/pit.h>

#include <stdbool.h>
#include <stddef.h>

#define TASK_STACK_SIZE 8192u

extern void context_switch(uint32_t *old_esp, uint32_t new_esp);

static task_t tasks[SCHED_MAX_TASKS];
static run_queue_t ready;
static task_t *current;
static task_t *idle_task;
static task_t *waiter;
static uint32_t live_tasks;
static uint32_t next_id;
static const sched_policy_t *policy = &sched_rr;
static bool sched_running;

static uint32_t trace_ids[SCHED_TRACE_MAX];
static uint32_t trace_len;
static bool tracing;

static void copy_name(char *dst, const char *src) {
    size_t i = 0;
    for (; src != NULL && src[i] != '\0' && i < TASK_NAME_MAX - 1u; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static task_t *alloc_slot(void) {
    for (uint32_t i = 0; i < SCHED_MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            return &tasks[i];
        }
    }
    return NULL;
}

static void init_task(task_t *t, const char *name, int priority, uint32_t burst) {
    t->id = next_id++;
    copy_name(t->name, name);
    t->priority = priority;
    t->burst = burst;
    t->remaining = 0;
    t->esp = 0;
    t->stack = NULL;
    t->entry = NULL;
    t->arg = NULL;
    t->wake_tick = 0;
    t->slice_left = 0;
    t->has_run = false;
    t->create_tick = pit_ticks();
    t->first_run_tick = 0;
    t->finish_tick = 0;
    t->run_ticks = 0;
    t->next = NULL;
}

// Must run with interrupts disabled.
static void schedule(void) {
    task_t *prev = current;

    if (prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
        if (prev != idle_task) {
            rq_push(&ready, prev);
        }
    }

    task_t *next = policy->pick(&ready);
    if (next == NULL) {
        next = idle_task;
    }

    next->state = TASK_RUNNING;
    next->slice_left = SCHED_QUANTUM_TICKS;
    if (!next->has_run) {
        next->has_run = true;
        next->first_run_tick = pit_ticks();
    }

    if (next == prev) {
        return;
    }
    current = next;
    context_switch(&prev->esp, next->esp);
}

// New tasks enter here from context_switch with interrupts still disabled.
static void task_trampoline(void) {
    sti();
    current->entry(current->arg);
    task_exit();
}

static void idle_main(void *arg) {
    (void)arg;
    for (;;) {
        hlt();
    }
}

static task_t *spawn(const char *name, void (*entry)(void *), void *arg, int priority,
                     uint32_t burst, bool runnable) {
    task_t *t = alloc_slot();
    if (t == NULL) {
        return NULL;
    }
    void *stack = kmalloc(TASK_STACK_SIZE);
    if (stack == NULL) {
        return NULL;
    }

    init_task(t, name, priority, burst);
    t->remaining = burst;
    t->stack = stack;
    t->entry = entry;
    t->arg = arg;

    // Initial frame popped by context_switch: edi, esi, ebx, ebp, return address.
    uint32_t *sp = (uint32_t *)((uint8_t *)stack + TASK_STACK_SIZE);
    *--sp = 0; // fake return address for the trampoline
    *--sp = (uint32_t)(uintptr_t)task_trampoline;
    *--sp = 0; // ebp
    *--sp = 0; // ebx
    *--sp = 0; // esi
    *--sp = 0; // edi
    t->esp = (uint32_t)(uintptr_t)sp;

    t->state = TASK_READY;
    if (runnable) {
        rq_push(&ready, t);
        live_tasks++;
    }
    return t;
}

void sched_init(const sched_policy_t *p) {
    uint32_t flags = irq_save();

    policy = p != NULL ? p : &sched_rr;
    rq_init(&ready);
    for (uint32_t i = 0; i < SCHED_MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
    }
    next_id = 0;
    live_tasks = 0;
    waiter = NULL;

    task_t *boot = &tasks[0];
    init_task(boot, "main", 0, 0);
    boot->state = TASK_RUNNING;
    boot->has_run = true;
    boot->slice_left = SCHED_QUANTUM_TICKS;
    current = boot;

    idle_task = spawn("idle", idle_main, NULL, 0, 0, false);
    if (idle_task == NULL) {
        panic("sched_init: cannot create idle task");
    }

    sched_running = true;
    irq_restore(flags);
}

const sched_policy_t *sched_policy(void) {
    return policy;
}

task_t *sched_current(void) {
    return current;
}

task_t *task_create(const char *name, void (*entry)(void *), void *arg, int priority,
                    uint32_t burst) {
    uint32_t flags = irq_save();
    task_t *t = spawn(name, entry, arg, priority, burst, true);
    irq_restore(flags);
    return t;
}

void task_exit(void) {
    cli();
    current->state = TASK_ZOMBIE;
    current->finish_tick = pit_ticks();
    live_tasks--;
    if (live_tasks == 0 && waiter != NULL) {
        waiter->state = TASK_READY;
        rq_push(&ready, waiter);
        waiter = NULL;
    }
    schedule();
    panic("task_exit: zombie was scheduled");
}

void task_sleep(uint32_t ticks) {
    uint32_t flags = irq_save();
    if (ticks > 0) {
        current->state = TASK_SLEEPING;
        current->wake_tick = pit_ticks() + ticks;
    }
    schedule();
    irq_restore(flags);
}

void sched_yield(void) {
    uint32_t flags = irq_save();
    schedule();
    irq_restore(flags);
}

void sched_block(void) {
    if (current->state == TASK_RUNNING) {
        current->state = TASK_BLOCKED;
    }
    schedule();
}

void sched_wakeup(task_t *t) {
    if (t == NULL || t->state != TASK_BLOCKED) {
        return;
    }
    t->state = TASK_READY;
    t->next = NULL;
    rq_push(&ready, t);
}

void sched_wait_all(void) {
    uint32_t flags = irq_save();
    if (live_tasks > 0) {
        waiter = current;
        current->state = TASK_BLOCKED;
        schedule();
    }
    // Exited tasks are off-CPU now, so their stacks can be freed.
    for (uint32_t i = 0; i < SCHED_MAX_TASKS; i++) {
        if (tasks[i].state == TASK_ZOMBIE && tasks[i].stack != NULL) {
            kfree(tasks[i].stack);
            tasks[i].stack = NULL;
        }
    }
    irq_restore(flags);
}

void sched_release(void) {
    uint32_t flags = irq_save();
    for (uint32_t i = 0; i < SCHED_MAX_TASKS; i++) {
        if (tasks[i].state == TASK_ZOMBIE && tasks[i].stack == NULL) {
            tasks[i].state = TASK_UNUSED;
        }
    }
    irq_restore(flags);
}

void sched_on_tick(void) {
    if (!sched_running) {
        return;
    }

    uint32_t now = pit_ticks();
    for (uint32_t i = 0; i < SCHED_MAX_TASKS; i++) {
        task_t *t = &tasks[i];
        if (t->state == TASK_SLEEPING && (int32_t)(now - t->wake_tick) >= 0) {
            t->state = TASK_READY;
            rq_push(&ready, t);
        }
    }

    if (current == idle_task) {
        if (ready.count > 0) {
            schedule();
        }
        return;
    }

    current->run_ticks++;
    if (tracing && trace_len < SCHED_TRACE_MAX &&
        (trace_len == 0 || trace_ids[trace_len - 1] != current->id)) {
        trace_ids[trace_len++] = current->id;
    }

    // A modeled burst just completed: let the task exit instead of requeueing it.
    if (current->remaining > 0 && --current->remaining == 0) {
        return;
    }

    if (policy->time_sliced && current->slice_left > 0 && --current->slice_left == 0) {
        schedule();
    }
}

void sched_trace_start(void) {
    trace_len = 0;
    tracing = true;
}

void sched_trace_stop(void) {
    tracing = false;
}

uint32_t sched_trace(const uint32_t **ids) {
    *ids = trace_ids;
    return trace_len;
}
