#include <kernix/sched_sim.h>

#include <stddef.h>

bool sim_run(const sched_policy_t *policy, const sim_task_t *tasks, uint32_t count,
             uint32_t quantum, sim_result_t *out) {
    task_t t[SIM_MAX_TASKS];
    run_queue_t rq;

    if (policy == NULL || tasks == NULL || out == NULL || count == 0 || count > SIM_MAX_TASKS) {
        return false;
    }

    out->task_count = count;
    out->end_time = 0;
    out->total_waiting = 0;
    out->total_turnaround = 0;
    out->total_response = 0;
    out->slice_count = 0;

    rq_init(&rq);
    for (uint32_t i = 0; i < count; i++) {
        t[i].id = i;
        t[i].priority = tasks[i].priority;
        t[i].burst = tasks[i].burst;
        t[i].remaining = tasks[i].burst;
        t[i].has_run = false;
        rq_push(&rq, &t[i]);
    }

    uint32_t now = 0;
    task_t *cur;
    while ((cur = policy->pick(&rq)) != NULL) {
        if (!cur->has_run) {
            cur->has_run = true;
            out->total_response += now;
        }

        // A task alone in the queue keeps the CPU until it finishes.
        uint32_t len = cur->remaining;
        if (policy->time_sliced && rq.count > 0 && quantum > 0 && quantum < len) {
            len = quantum;
        }

        if (out->slice_count >= SIM_MAX_SLICES) {
            return false;
        }
        sim_slice_t *s = &out->slices[out->slice_count++];
        s->task = cur->id;
        s->remaining = cur->remaining;
        s->len = len;

        now += len;
        cur->remaining -= len;
        if (cur->remaining == 0) {
            out->total_turnaround += now;
            out->total_waiting += now - cur->burst;
        } else {
            rq_push(&rq, cur);
        }
    }

    out->end_time = now;
    return true;
}
