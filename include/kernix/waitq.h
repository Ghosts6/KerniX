#ifndef KERNIX_WAITQ_H
#define KERNIX_WAITQ_H

#include <kernix/spinlock.h>
#include <kernix/task.h>

// FIFO of blocked tasks. task_t::next is reused while TASK_BLOCKED.
typedef struct wait_queue {
    spinlock_t lock;
    task_t *head;
    task_t *tail;
} wait_queue_t;

void waitq_init(wait_queue_t *wq);

// Enqueue the current task and schedule away. Returns after a wake.
void waitq_sleep(wait_queue_t *wq);

// Caller holds wq->lock with IRQs off. Sleeps and re-acquires the lock on wake.
void waitq_sleep_locked(wait_queue_t *wq);

// Move the oldest waiter to the ready queue (no-op if empty).
void waitq_wake_one(wait_queue_t *wq);

// Caller holds wq->lock with IRQs off.
void waitq_wake_one_locked(wait_queue_t *wq);

// Wake every waiter.
void waitq_wake_all(wait_queue_t *wq);

#endif
