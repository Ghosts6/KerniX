#include <kernix/sched_policy.h>

#include <stddef.h>

void rq_init(run_queue_t *rq) {
    rq->head = NULL;
    rq->tail = NULL;
    rq->count = 0;
}

void rq_push(run_queue_t *rq, task_t *task) {
    task->next = NULL;
    if (rq->tail != NULL) {
        rq->tail->next = task;
    } else {
        rq->head = task;
    }
    rq->tail = task;
    rq->count++;
}

void rq_remove(run_queue_t *rq, task_t *task) {
    task_t *prev = NULL;
    for (task_t *cur = rq->head; cur != NULL; prev = cur, cur = cur->next) {
        if (cur != task) {
            continue;
        }
        if (prev != NULL) {
            prev->next = cur->next;
        } else {
            rq->head = cur->next;
        }
        if (rq->tail == cur) {
            rq->tail = prev;
        }
        cur->next = NULL;
        rq->count--;
        return;
    }
}

task_t *rq_pop(run_queue_t *rq) {
    task_t *task = rq->head;
    if (task != NULL) {
        rq_remove(rq, task);
    }
    return task;
}

static task_t *pick_first(run_queue_t *rq) {
    return rq_pop(rq);
}

// Strict comparisons keep the earliest queued task on ties, as in the course.
static task_t *pick_shortest(run_queue_t *rq) {
    task_t *best = NULL;
    for (task_t *t = rq->head; t != NULL; t = t->next) {
        if (best == NULL || t->burst < best->burst) {
            best = t;
        }
    }
    if (best != NULL) {
        rq_remove(rq, best);
    }
    return best;
}

static task_t *pick_highest(run_queue_t *rq) {
    task_t *best = NULL;
    for (task_t *t = rq->head; t != NULL; t = t->next) {
        if (best == NULL || t->priority > best->priority) {
            best = t;
        }
    }
    if (best != NULL) {
        rq_remove(rq, best);
    }
    return best;
}

const sched_policy_t sched_fcfs = {"fcfs", false, pick_first};
const sched_policy_t sched_sjf = {"sjf", false, pick_shortest};
const sched_policy_t sched_priority = {"priority", false, pick_highest};
const sched_policy_t sched_rr = {"rr", true, pick_first};
// Preempted tasks requeue at the tail, so equal priorities share round-robin.
const sched_policy_t sched_priority_rr = {"priority_rr", true, pick_highest};

static bool name_matches(const char *want, const char *s) {
    while (*want != '\0' && *s == *want) {
        want++;
        s++;
    }
    return *want == '\0' && (*s == '\0' || *s == ' ' || *s == '\t');
}

const sched_policy_t *sched_policy_find(const char *name) {
    static const sched_policy_t *const all[] = {
        &sched_fcfs, &sched_sjf, &sched_priority, &sched_rr, &sched_priority_rr,
    };

    if (name == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        if (name_matches(all[i]->name, name)) {
            return all[i];
        }
    }
    return NULL;
}
