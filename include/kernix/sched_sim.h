#ifndef KERNIX_SCHED_SIM_H
#define KERNIX_SCHED_SIM_H

#include <kernix/sched_policy.h>

#include <stdbool.h>
#include <stdint.h>

#define SIM_MAX_TASKS  16
#define SIM_MAX_SLICES 128

typedef struct sim_task {
    const char *name;
    int priority;
    uint32_t burst;
} sim_task_t;

typedef struct sim_slice {
    uint32_t task;      // index into the input array
    uint32_t remaining; // burst left before this slice
    uint32_t len;
} sim_slice_t;

typedef struct sim_result {
    uint32_t task_count;
    uint32_t end_time;
    uint32_t total_waiting;
    uint32_t total_turnaround;
    uint32_t total_response;
    uint32_t slice_count;
    sim_slice_t slices[SIM_MAX_SLICES];
} sim_result_t;

// Course model: every task arrives at time 0, in input order.
bool sim_run(const sched_policy_t *policy, const sim_task_t *tasks, uint32_t count,
             uint32_t quantum, sim_result_t *out);

#endif
