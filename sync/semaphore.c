#include <kernix/semaphore.h>

void sem_init(semaphore_t *s, int value) {
    s->count = value;
    waitq_init(&s->waiters);
}

void sem_wait(semaphore_t *s) {
    uint32_t flags = spin_lock_irqsave(&s->waiters.lock);
    while (s->count <= 0) {
        waitq_sleep_locked(&s->waiters);
    }
    s->count--;
    spin_unlock_irqrestore(&s->waiters.lock, flags);
}

void sem_post(semaphore_t *s) {
    uint32_t flags = spin_lock_irqsave(&s->waiters.lock);
    s->count++;
    waitq_wake_one_locked(&s->waiters);
    spin_unlock_irqrestore(&s->waiters.lock, flags);
}
