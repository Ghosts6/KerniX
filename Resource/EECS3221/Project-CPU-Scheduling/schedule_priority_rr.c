#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "task.h"
#include "list.h"
#include "schedulers.h"
#include "cpu.h"

// Priority with Round Robin Scheduling

typedef struct {
    int task_idx;
    int remaining_burst;
} QueueEntry;

// Tasks are kept in input order in a dynamic array. response_recorded[i]
// and remaining_burst[i] track tasks[i] across priority levels.
static Task **tasks = NULL;
static int task_count = 0;
static int capacity = 10;
static int *response_recorded = NULL;
static int *remaining_burst = NULL;

/**
 * Called by driver.c once per input line. Copies the task into the array,
 * doubling capacity (for all arrays) when full so any number of tasks works.
 */
void add(char *name, int priority, int burst) {
    if (tasks == NULL) {
        tasks = malloc(sizeof(Task*) * capacity);
        response_recorded = malloc(sizeof(int) * capacity);
        remaining_burst = malloc(sizeof(int) * capacity);
    }
    if (task_count >= capacity) {
        capacity *= 2;
        tasks = realloc(tasks, sizeof(Task*) * capacity);
        response_recorded = realloc(response_recorded, sizeof(int) * capacity);
        remaining_burst = realloc(remaining_burst, sizeof(int) * capacity);
    }

    Task *newTask = malloc(sizeof(Task));
    newTask->name = malloc(strlen(name) + 1);
    strcpy(newTask->name, name);
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->tid = task_count;

    tasks[task_count] = newTask;
    response_recorded[task_count] = 0;
    remaining_burst[task_count] = burst;
    task_count++;
}

/**
 * For each priority level, from MAX_PRIORITY down to MIN_PRIORITY, builds a
 * queue of that level's tasks in input order and runs round-robin on it
 * until the level is empty, using the same circular-queue approach as RR.
 * Metrics (all tasks arrive at 0):
 *   response   = time of first slice
 *   turnaround = completion time
 *   waiting    = turnaround - original burst
 */
void schedule() {
    int current_time = 0;
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;

    for (int priority = MAX_PRIORITY; priority >= MIN_PRIORITY; priority--) {
        // Gather this level's tasks; scanning in index order keeps input order.
        QueueEntry *queue = malloc(sizeof(QueueEntry) * task_count);
        int queue_size = 0;

        for (int i = 0; i < task_count; i++) {
            if (tasks[i]->priority == priority && remaining_burst[i] > 0) {
                queue[queue_size].task_idx = i;
                queue[queue_size].remaining_burst = remaining_burst[i];
                queue_size++;
            }
        }

        int queue_front = 0;

        while (queue_size > 0) {
            QueueEntry *current = &queue[queue_front];
            int task_id = current->task_idx;
            Task *current_task = tasks[task_id];

            // Response time is only counted on the task's first slice.
            if (!response_recorded[task_id]) {
                response_recorded[task_id] = 1;
                total_response += current_time;
            }

            // Only one task left at this level: no one to share with, so run
            // it to completion (matches correct-priority_rr-output.txt).
            // Otherwise run a quantum, or the remainder if shorter.
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
            remaining_burst[task_id] -= time_slice;
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

        free(queue);
    }

    printf("\n");
    printf("Average waiting time = %.2f\n", total_waiting / task_count);
    printf("Average turnaround time = %.2f\n", total_turnaround / task_count);
    printf("Average response time = %.2f\n", total_response / task_count);
}
