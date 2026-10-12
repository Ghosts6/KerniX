#include <kernix/sched_sim.h>

#include <stdint.h>
#include <stdio.h>

// Course-style input (name, priority, burst); all tasks arrive at 0.
static const sim_task_t course_tasks[] = {
    {"T1", 4, 20}, {"T2", 2, 25}, {"T3", 3, 25}, {"T4", 3, 15}, {"T5", 10, 10}, {"T6", 1, 10},
};
#define COURSE_COUNT (sizeof(course_tasks) / sizeof(course_tasks[0]))
#define COURSE_QUANTUM 10

static int failures;
static sim_result_t res;

static void expect_true(const char *name, int cond) {
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", name);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

static void expect_eq_u32(const char *name, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %u want %u\n", name, got, want);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

static void check_totals(const sched_policy_t *p, uint32_t waiting, uint32_t turnaround,
                         uint32_t response) {
    char label[64];
    int ok = sim_run(p, course_tasks, COURSE_COUNT, COURSE_QUANTUM, &res);
    snprintf(label, sizeof(label), "%s runs", p->name);
    expect_true(label, ok);
    snprintf(label, sizeof(label), "%s end time", p->name);
    expect_eq_u32(label, res.end_time, 105);
    snprintf(label, sizeof(label), "%s total waiting", p->name);
    expect_eq_u32(label, res.total_waiting, waiting);
    snprintf(label, sizeof(label), "%s total turnaround", p->name);
    expect_eq_u32(label, res.total_turnaround, turnaround);
    snprintf(label, sizeof(label), "%s total response", p->name);
    expect_eq_u32(label, res.total_response, response);
}

static void check_order(const char *name, const uint32_t *tasks, const uint32_t *lens,
                        uint32_t n) {
    int ok = res.slice_count == n;
    for (uint32_t i = 0; ok && i < n; i++) {
        ok = res.slices[i].task == tasks[i] && res.slices[i].len == lens[i];
    }
    if (!ok) {
        fprintf(stderr, "  got:");
        for (uint32_t i = 0; i < res.slice_count; i++) {
            fprintf(stderr, " T%u/%u", res.slices[i].task + 1, res.slices[i].len);
        }
        fprintf(stderr, "\n");
    }
    expect_true(name, ok);
}

int main(void) {
    // Expected totals worked out by hand from the course algorithms.
    check_totals(&sched_fcfs, 315, 420, 315);
    check_totals(&sched_sjf, 200, 305, 200);
    check_totals(&sched_priority, 260, 365, 260);
    check_totals(&sched_rr, 375, 480, 150);
    check_totals(&sched_priority_rr, 270, 375, 245);

    sim_run(&sched_fcfs, course_tasks, COURSE_COUNT, COURSE_QUANTUM, &res);
    check_order("fcfs order", (const uint32_t[]){0, 1, 2, 3, 4, 5},
                (const uint32_t[]){20, 25, 25, 15, 10, 10}, 6);

    // SJF ties (T5/T6 at 10, T2/T3 at 25) go to the earlier task.
    sim_run(&sched_sjf, course_tasks, COURSE_COUNT, COURSE_QUANTUM, &res);
    check_order("sjf order", (const uint32_t[]){4, 5, 3, 0, 1, 2},
                (const uint32_t[]){10, 10, 15, 20, 25, 25}, 6);

    sim_run(&sched_priority, course_tasks, COURSE_COUNT, COURSE_QUANTUM, &res);
    check_order("priority order", (const uint32_t[]){4, 0, 2, 3, 1, 5},
                (const uint32_t[]){10, 20, 25, 15, 25, 10}, 6);

    sim_run(&sched_rr, course_tasks, COURSE_COUNT, COURSE_QUANTUM, &res);
    check_order("rr order",
                (const uint32_t[]){0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 1, 2},
                (const uint32_t[]){10, 10, 10, 10, 10, 10, 10, 10, 10, 5, 5, 5}, 12);

    // T5 and T1 are alone in their levels; T3/T4 share level 3 round-robin.
    sim_run(&sched_priority_rr, course_tasks, COURSE_COUNT, COURSE_QUANTUM, &res);
    check_order("priority_rr order",
                (const uint32_t[]){4, 0, 0, 2, 3, 2, 3, 2, 1, 1, 1, 5},
                (const uint32_t[]){10, 10, 10, 10, 10, 10, 5, 5, 10, 10, 5, 10}, 12);

    expect_true("find rr", sched_policy_find("rr") == &sched_rr);
    expect_true("find priority_rr", sched_policy_find("priority_rr") == &sched_priority_rr);
    expect_true("find stops at space", sched_policy_find("sjf fault=de") == &sched_sjf);
    expect_true("find rejects prefix", sched_policy_find("r") == NULL);
    expect_true("find rejects suffix", sched_policy_find("rrx") == NULL);
    expect_true("find rejects unknown", sched_policy_find("lottery") == NULL);

    run_queue_t rq;
    task_t a = {0}, b = {0}, c = {0};
    rq_init(&rq);
    rq_push(&rq, &a);
    rq_push(&rq, &b);
    rq_push(&rq, &c);
    rq_remove(&rq, &c);
    rq_push(&rq, &c);
    expect_eq_u32("rq count", rq.count, 3);
    expect_true("rq pop order", rq_pop(&rq) == &a && rq_pop(&rq) == &b && rq_pop(&rq) == &c);
    expect_true("rq empty", rq_pop(&rq) == NULL && rq.head == NULL && rq.tail == NULL);

    if (failures != 0) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("all sched tests passed\n");
    return 0;
}
