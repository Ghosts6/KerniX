#ifndef KERNIX_TASK_H
#define KERNIX_TASK_H

#include <stdbool.h>
#include <stdint.h>

#define TASK_NAME_MAX 16

typedef enum task_state {
    TASK_UNUSED,
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_BLOCKED,
    TASK_ZOMBIE,
} task_state_t;

typedef struct task {
    uint32_t id;
    char name[TASK_NAME_MAX];
    task_state_t state;
    int priority;       // higher runs first
    uint32_t burst;     // expected run time in ticks (SJF key)
    uint32_t remaining; // modeled CPU burst left in ticks; 0 = not modeled
    uint32_t esp;       // saved stack pointer while switched out
    void *stack;
    uint32_t kstack_top; // TSS.esp0 for user tasks (top of kernel stack)
    uint32_t cr3;        // page directory phys; 0 means kernel PD
    bool user;
    uint32_t user_eip;
    uint32_t user_esp;
    void (*entry)(void *);
    void *arg;
    uint32_t wake_tick;
    uint32_t slice_left;
    bool has_run;
    uint32_t create_tick;
    uint32_t first_run_tick;
    uint32_t finish_tick;
    uint32_t run_ticks;
    struct task *next;
} task_t;

// FIFO of ready tasks; policies may remove from anywhere.
typedef struct run_queue {
    task_t *head;
    task_t *tail;
    uint32_t count;
} run_queue_t;

void rq_init(run_queue_t *rq);
void rq_push(run_queue_t *rq, task_t *task);
void rq_remove(run_queue_t *rq, task_t *task);
task_t *rq_pop(run_queue_t *rq);

#endif
