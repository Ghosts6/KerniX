#include <kernix/mutex.h>

void mutex_init(mutex_t *m) {
    m->held = 0;
    waitq_init(&m->waiters);
}

void mutex_lock(mutex_t *m) {
    uint32_t flags = spin_lock_irqsave(&m->waiters.lock);
    while (m->held) {
        waitq_sleep_locked(&m->waiters);
    }
    m->held = 1;
    spin_unlock_irqrestore(&m->waiters.lock, flags);
}

void mutex_unlock(mutex_t *m) {
    uint32_t flags = spin_lock_irqsave(&m->waiters.lock);
    m->held = 0;
    waitq_wake_one_locked(&m->waiters);
    spin_unlock_irqrestore(&m->waiters.lock, flags);
}
