#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "task.h"
#include "list.h"
#include "schedulers.h"
#include "cpu.h"

// Round Robin Scheduling

typedef struct {
    int task_idx;
    int remaining_burst;
    int response_recorded;
} QueueNode;

static Task **tasks = NULL;
static int task_count = 0;
static int capacity = 10;
static int *response_times = NULL;

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

void schedule() {
    // Create queue and initialize remaining burst times
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

        // Record response time on first execution
        if (!current->response_recorded) {
            current->response_recorded = 1;
            response_times[current->task_idx] = current_time;
            total_response += current_time;
        }

        // Execute for time slice
        int time_slice = QUANTUM;
        if (current->remaining_burst < QUANTUM) {
            time_slice = current->remaining_burst;
        }

        // Show remaining burst before execution
        int original_burst = current_task->burst;
        current_task->burst = current->remaining_burst;
        run(current_task, time_slice);
        current_task->burst = original_burst;

        current->remaining_burst -= time_slice;
        current_time += time_slice;

        // If task is done
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

    printf("\n");
    printf("Average waiting time = %.2f\n", total_waiting / task_count);
    printf("Average turnaround time = %.2f\n", total_turnaround / task_count);
    printf("Average response time = %.2f\n", total_response / task_count);

    free(queue);
}
