#ifndef KERNIX_SCHED_H
#define KERNIX_SCHED_H

#include <kernix/sched_policy.h>
#include <kernix/task.h>

#include <stdint.h>

#define SCHED_MAX_TASKS     16
#define SCHED_QUANTUM_TICKS 2
#define SCHED_TRACE_MAX     64

// Turns the running kmain context into task 0 and starts the idle task.
void sched_init(const sched_policy_t *policy);
const sched_policy_t *sched_policy(void);
task_t *sched_current(void);

task_t *task_create(const char *name, void (*entry)(void *), void *arg, int priority,
                    uint32_t burst);
// Create a ring-3 task; entry is via iret into user_eip/user_esp with CR3=cr3.
task_t *task_create_user(const char *name, uint32_t cr3, uint32_t user_eip, uint32_t user_esp);
void task_exit(void) __attribute__((noreturn));
void task_sleep(uint32_t ticks);
void sched_yield(void);

// Block the current task (IRQs must be off). Returns after sched_wakeup.
void sched_block(void);
// Mark a blocked task ready and enqueue it (IRQs must be off).
void sched_wakeup(task_t *t);

// Block the caller until every created task has exited, then free their stacks.
// Only one task may wait at a time.
void sched_wait_all(void);
// Return exited task slots to the pool once their stats have been read.
void sched_release(void);

// Called from the PIT IRQ after EOI; may switch tasks.
void sched_on_tick(void);

// Record which task consumed each tick (consecutive repeats collapsed).
void sched_trace_start(void);
void sched_trace_stop(void);
uint32_t sched_trace(const uint32_t **ids);

#endif
