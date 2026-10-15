#include <kernix/waitq.h>
#include <kernix/sched.h>

#include <stddef.h>

void waitq_init(wait_queue_t *wq) {
    spin_lock_init(&wq->lock);
    wq->head = NULL;
    wq->tail = NULL;
}

static void enqueue_locked(wait_queue_t *wq, task_t *t) {
    t->next = NULL;
    if (wq->tail != NULL) {
        wq->tail->next = t;
    } else {
        wq->head = t;
    }
    wq->tail = t;
}

static task_t *dequeue_locked(wait_queue_t *wq) {
    task_t *t = wq->head;
    if (t == NULL) {
        return NULL;
    }
    wq->head = t->next;
    if (wq->head == NULL) {
        wq->tail = NULL;
    }
    t->next = NULL;
    return t;
}

void waitq_sleep_locked(wait_queue_t *wq) {
    task_t *self = sched_current();
    enqueue_locked(wq, self);
    self->state = TASK_BLOCKED;
    spin_unlock(&wq->lock);
    sched_block();
    spin_lock(&wq->lock);
}

void waitq_sleep(wait_queue_t *wq) {
    uint32_t flags = spin_lock_irqsave(&wq->lock);
    waitq_sleep_locked(wq);
    spin_unlock_irqrestore(&wq->lock, flags);
}

void waitq_wake_one_locked(wait_queue_t *wq) {
    task_t *t = dequeue_locked(wq);
    if (t != NULL) {
        sched_wakeup(t);
    }
}

void waitq_wake_one(wait_queue_t *wq) {
    uint32_t flags = spin_lock_irqsave(&wq->lock);
    waitq_wake_one_locked(wq);
    spin_unlock_irqrestore(&wq->lock, flags);
}

void waitq_wake_all(wait_queue_t *wq) {
    uint32_t flags = spin_lock_irqsave(&wq->lock);
    for (;;) {
        task_t *t = dequeue_locked(wq);
        if (t == NULL) {
            break;
        }
        sched_wakeup(t);
    }
    spin_unlock_irqrestore(&wq->lock, flags);
}
