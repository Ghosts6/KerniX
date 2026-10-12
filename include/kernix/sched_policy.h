#ifndef KERNIX_SCHED_POLICY_H
#define KERNIX_SCHED_POLICY_H

#include <kernix/task.h>

#include <stdbool.h>

typedef struct sched_policy {
    const char *name;
    bool time_sliced; // preempt when the quantum expires
    // Remove and return the next task to run, or NULL if the queue is empty.
    task_t *(*pick)(run_queue_t *rq);
} sched_policy_t;

extern const sched_policy_t sched_fcfs;
extern const sched_policy_t sched_sjf;
extern const sched_policy_t sched_priority;
extern const sched_policy_t sched_rr;
extern const sched_policy_t sched_priority_rr;

// Match a policy name terminated by NUL, space or tab (e.g. a cmdline value).
const sched_policy_t *sched_policy_find(const char *name);

#endif
