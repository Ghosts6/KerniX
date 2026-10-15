#ifndef KERNIX_MUTEX_H
#define KERNIX_MUTEX_H

#include <kernix/waitq.h>

// held is protected by waiters.lock.
typedef struct mutex {
    int held;
    wait_queue_t waiters;
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif
