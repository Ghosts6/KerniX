#include <kernix/sched_selftest.h>
#include <kernix/sched.h>
#include <kernix/sched_sim.h>
#include <kernix/cpu.h>
#include <kernix/kprintf.h>
#include <kernix/pit.h>

#include <stdint.h>

// Course input scaled down: bursts in timer ticks, quantum 2 ticks (course: 10 units).
static const sim_task_t demo_tasks[] = {
    {"T1", 4, 4}, {"T2", 2, 5}, {"T3", 3, 5}, {"T4", 3, 3}, {"T5", 10, 2}, {"T6", 1, 2},
};
#define DEMO_COUNT (sizeof(demo_tasks) / sizeof(demo_tasks[0]))

#define YIELD_ROUNDS 3
#define SLEEP_TICKS  3

static sim_result_t sim;

static void print_avg(const char *label, uint32_t total, uint32_t count) {
    uint32_t hundredths = (total * 100u + count / 2u) / count;
    kprintf("Average %s time = %u.%u%u\n", label, hundredths / 100u, (hundredths / 10u) % 10u,
            hundredths % 10u);
}

static void print_sim(const sched_policy_t *policy) {
    kprintf("sched: simulated %s (quantum %u)\n", policy->name, SCHED_QUANTUM_TICKS);
    for (uint32_t i = 0; i < sim.slice_count; i++) {
        const sim_slice_t *s = &sim.slices[i];
        const sim_task_t *t = &demo_tasks[s->task];
        kprintf("Running task = [%s] [%d] [%u] for %u units.\n", t->name, t->priority,
                s->remaining, s->len);
    }
    print_avg("waiting", sim.total_waiting, sim.task_count);
    print_avg("turnaround", sim.total_turnaround, sim.task_count);
    print_avg("response", sim.total_response, sim.task_count);
}

static void demo_worker(void *arg) {
    (void)arg;
    task_t *self = sched_current();
    while (*(volatile uint32_t *)&self->remaining > 0) {
        __asm__ volatile("pause");
    }
}

static bool within(uint32_t got, uint32_t want, uint32_t slack) {
    uint32_t diff = got > want ? got - want : want - got;
    return diff <= slack;
}

// Let any tasks already created run to completion, then free their slots.
static void drain_tasks(uint32_t flags) {
    sched_wait_all();
    irq_restore(flags);
    sched_release();
}

static void print_trace(const char *label, const uint32_t *ids, uint32_t n) {
    kprintf("  %s:", label);
    for (uint32_t i = 0; i < n; i++) {
        kprintf(" %u", ids[i]);
    }
    kprintf("\n");
}

static bool run_policy_demo(void) {
    const sched_policy_t *policy = sched_policy();
    task_t *workers[DEMO_COUNT];

    if (!sim_run(policy, demo_tasks, DEMO_COUNT, SCHED_QUANTUM_TICKS, &sim)) {
        kprintf("sched demo FAIL simulator\n");
        return false;
    }
    print_sim(policy);

    // Create everything before any worker runs so queue order matches input order.
    uint32_t flags = irq_save();
    for (uint32_t i = 0; i < DEMO_COUNT; i++) {
        workers[i] = task_create(demo_tasks[i].name, demo_worker, 0, demo_tasks[i].priority,
                                 demo_tasks[i].burst);
        if (workers[i] == 0) {
            drain_tasks(flags);
            kprintf("sched demo FAIL task_create\n");
            return false;
        }
    }
    sched_trace_start();
    sched_wait_all();
    sched_trace_stop();
    irq_restore(flags);

    // Expected: simulator slices mapped to task ids, consecutive repeats collapsed.
    uint32_t want[SIM_MAX_SLICES];
    uint32_t want_len = 0;
    for (uint32_t i = 0; i < sim.slice_count; i++) {
        uint32_t id = workers[sim.slices[i].task]->id;
        if (want_len == 0 || want[want_len - 1] != id) {
            want[want_len++] = id;
        }
    }

    const uint32_t *got;
    uint32_t got_len = sched_trace(&got);
    bool match = got_len == want_len;
    for (uint32_t i = 0; match && i < got_len; i++) {
        match = got[i] == want[i];
    }

    uint32_t waiting = 0, turnaround = 0, response = 0;
    for (uint32_t i = 0; i < DEMO_COUNT; i++) {
        const task_t *t = workers[i];
        uint32_t tat = t->finish_tick - t->create_tick;
        turnaround += tat;
        response += t->first_run_tick - t->create_tick;
        waiting += tat > t->run_ticks ? tat - t->run_ticks : 0;
    }
    kprintf("sched: kernel threads (ticks)\n");
    print_avg("waiting", waiting, DEMO_COUNT);
    print_avg("turnaround", turnaround, DEMO_COUNT);
    print_avg("response", response, DEMO_COUNT);

    sched_release();

    if (!match) {
        kprintf("sched demo FAIL policy=%s order mismatch\n", policy->name);
        print_trace("want", want, want_len);
        print_trace("got ", got, got_len);
        return false;
    }
    // Allow one tick of timer jitter per task (e.g. under emulated QEMU in CI).
    if (!within(waiting, sim.total_waiting, DEMO_COUNT) ||
        !within(turnaround, sim.total_turnaround, DEMO_COUNT) ||
        !within(response, sim.total_response, DEMO_COUNT)) {
        kprintf("sched demo FAIL policy=%s stats differ from simulator\n", policy->name);
        return false;
    }
    kprintf("sched demo ok policy=%s slices=%u\n", policy->name, got_len);
    return true;
}

static char yield_log[2 * YIELD_ROUNDS + 1];
static uint32_t yield_len;

static void yielder(void *arg) {
    char tag = (char)(uintptr_t)arg;
    for (int i = 0; i < YIELD_ROUNDS; i++) {
        uint32_t flags = irq_save();
        yield_log[yield_len++] = tag;
        irq_restore(flags);
        sched_yield();
    }
}

static bool run_yield_test(void) {
    yield_len = 0;

    uint32_t flags = irq_save();
    task_t *a = task_create("ya", yielder, (void *)(uintptr_t)'A', 5, 0);
    task_t *b = task_create("yb", yielder, (void *)(uintptr_t)'B', 5, 0);
    drain_tasks(flags);
    if (a == 0 || b == 0) {
        kprintf("sched yield FAIL task_create\n");
        return false;
    }

    yield_log[yield_len] = '\0';
    const char *want = "ABABAB";
    bool ok = yield_len == 2 * YIELD_ROUNDS;
    for (uint32_t i = 0; ok && i < yield_len; i++) {
        ok = yield_log[i] == want[i];
    }
    if (!ok) {
        kprintf("sched yield FAIL got=%s want=%s\n", yield_log, want);
        return false;
    }
    kprintf("sched yield ok order=%s\n", yield_log);
    return true;
}

static volatile uint32_t slept_ticks;

static void sleeper(void *arg) {
    uint32_t start = pit_ticks();
    task_sleep((uint32_t)(uintptr_t)arg);
    slept_ticks = pit_ticks() - start;
}

static bool run_sleep_test(void) {
    slept_ticks = 0;

    uint32_t flags = irq_save();
    task_t *t = task_create("sleeper", sleeper, (void *)(uintptr_t)SLEEP_TICKS, 5, 0);
    drain_tasks(flags);
    if (t == 0) {
        kprintf("sched sleep FAIL task_create\n");
        return false;
    }

    if (slept_ticks < SLEEP_TICKS) {
        kprintf("sched sleep FAIL slept=%u want>=%u\n", slept_ticks, SLEEP_TICKS);
        return false;
    }
    kprintf("sched sleep ok ticks=%u\n", slept_ticks);
    return true;
}

bool sched_selftest(void) {
    bool ok = run_policy_demo();
    ok = run_yield_test() && ok;
    ok = run_sleep_test() && ok;
    return ok;
}
