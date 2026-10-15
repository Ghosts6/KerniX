#ifndef KERNIX_SEMAPHORE_H
#define KERNIX_SEMAPHORE_H

#include <kernix/waitq.h>

// count is protected by waiters.lock.
typedef struct semaphore {
    int count;
    wait_queue_t waiters;
} semaphore_t;

void sem_init(semaphore_t *s, int value);
void sem_wait(semaphore_t *s);
void sem_post(semaphore_t *s);

#endif
