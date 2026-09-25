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

static Task **tasks = NULL;
static int task_count = 0;
static int capacity = 10;
static int *response_recorded = NULL;
static int *remaining_burst = NULL;

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

void schedule() {
    int current_time = 0;
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;

    // Process by priority level (highest to lowest)
    for (int priority = MAX_PRIORITY; priority >= MIN_PRIORITY; priority--) {
        // Create queue for tasks with this priority (maintain input order)
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

        // Process all tasks at this priority level with RR
        while (queue_size > 0) {
            QueueEntry *current = &queue[queue_front];
            int task_id = current->task_idx;
            Task *current_task = tasks[task_id];

            // Record response time on first execution
            if (!response_recorded[task_id]) {
                response_recorded[task_id] = 1;
                total_response += current_time;
            }

            // If only one task left in queue, run to completion; otherwise use quantum
            int time_slice = current->remaining_burst;
            if (queue_size > 1) {  // More than one task in queue
                time_slice = QUANTUM;
                if (current->remaining_burst < QUANTUM) {
                    time_slice = current->remaining_burst;
                }
            }

            // Show remaining burst before execution
            int original_burst = current_task->burst;
            current_task->burst = current->remaining_burst;
            run(current_task, time_slice);
            current_task->burst = original_burst;

            current->remaining_burst -= time_slice;
            remaining_burst[task_id] -= time_slice;
            current_time += time_slice;

            // If task is done, remove from queue and record metrics
            if (current->remaining_burst == 0) {
                int completion_time = current_time;
                int turnaround_time = completion_time;
                int waiting_time = turnaround_time - current_task->burst;

                total_turnaround += turnaround_time;
                total_waiting += waiting_time;

                // Remove this task from queue
                for (int i = queue_front; i < queue_size - 1; i++) {
                    queue[i] = queue[i + 1];
                }
                queue_size--;

                if (queue_size > 0 && queue_front >= queue_size) {
                    queue_front = 0;
                }
            } else {
                // Move to next task in queue
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
