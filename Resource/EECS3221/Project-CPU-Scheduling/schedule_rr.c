#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "task.h"
#include "list.h"
#include "schedulers.h"
#include "cpu.h"

// One ready-queue entry: which task it is, how much CPU time it still needs,
// and whether it has run yet (for response time).
typedef struct {
    int task_idx;
    int remaining_burst;
    int response_recorded;
} QueueNode;

// Tasks are kept in input order in a dynamic array; response_times[i] holds
// the first start time of tasks[i] (-1 until it first runs).
static Task **tasks = NULL;
static int task_count = 0;
static int capacity = 10;
static int *response_times = NULL;

/**
 * Called by driver.c once per input line. Copies the task into the array,
 * doubling capacity (for both arrays) when full so any number of tasks works.
 */
void add(char *name, int priority, int burst) {
    if (tasks == NULL) {
        tasks = malloc(sizeof(Task*) * capacity);
        response_times = malloc(sizeof(int) * capacity);
    }
    if (task_count >= capacity) {
        capacity *= 2;
        tasks = realloc(tasks, sizeof(Task*) * capacity);
        response_times = realloc(response_times, sizeof(int) * capacity);
    }

    Task *newTask = malloc(sizeof(Task));
    newTask->name = malloc(strlen(name) + 1);
    strcpy(newTask->name, name);
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->tid = task_count;

    tasks[task_count] = newTask;
    response_times[task_count] = -1;
    task_count++;
}

/**
 * Simulates RR with an array used as a circular queue: queue_front is the
 * task whose turn it is, and it advances (wrapping to 0) after each slice.
 * Metrics (all tasks arrive at 0):
 *   response   = time of first slice
 *   turnaround = completion time
 *   waiting    = turnaround - original burst
 */
void schedule() {
    // Build the ready queue in input order with each task's full burst.
    QueueNode *queue = malloc(sizeof(QueueNode) * task_count);
    for (int i = 0; i < task_count; i++) {
        queue[i].task_idx = i;
        queue[i].remaining_burst = tasks[i]->burst;
        queue[i].response_recorded = 0;
    }

    int queue_size = task_count;
    int current_time = 0;
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;

    int queue_front = 0;

    while (queue_size > 0) {
        QueueNode *current = &queue[queue_front];
        Task *current_task = tasks[current->task_idx];

        // Response time is only counted on the task's first slice.
        if (!current->response_recorded) {
            current->response_recorded = 1;
            response_times[current->task_idx] = current_time;
            total_response += current_time;
        }

        // Only one task left: no one to switch to when the quantum expires,
        // so it keeps the CPU and runs to completion (same rule as
        // Priority_RR). Otherwise run a quantum, or the remainder if shorter.
        int time_slice = current->remaining_burst;
        if (queue_size > 1) {
            time_slice = QUANTUM;
            if (current->remaining_burst < QUANTUM) {
                time_slice = current->remaining_burst;
            }
        }

        // run() prints task->burst, but the output must show the remaining
        // burst, so swap it in for the call and restore the original after.
        int original_burst = current_task->burst;
        current_task->burst = current->remaining_burst;
        run(current_task, time_slice);
        current_task->burst = original_burst;

        current->remaining_burst -= time_slice;
        current_time += time_slice;

        if (current->remaining_burst == 0) {
            int completion_time = current_time;
            int turnaround_time = completion_time;
            int waiting_time = turnaround_time - current_task->burst;

            total_turnaround += turnaround_time;
            total_waiting += waiting_time;

            // Remove the finished task by shifting later entries left. The
            // next task slides into queue_front, so the index stays put
            // unless we removed the last entry, in which case wrap to 0.
            for (int i = queue_front; i < queue_size - 1; i++) {
                queue[i] = queue[i + 1];
            }
            queue_size--;

            if (queue_size > 0 && queue_front >= queue_size) {
                queue_front = 0;
            }
        } else {
            // Unfinished: it keeps its place and the next task gets a turn.
            queue_front = (queue_front + 1) % queue_size;
        }
    }

    printf("\n");
    printf("Average waiting time = %.2f\n", total_waiting / task_count);
    printf("Average turnaround time = %.2f\n", total_turnaround / task_count);
    printf("Average response time = %.2f\n", total_response / task_count);

    free(queue);
}
