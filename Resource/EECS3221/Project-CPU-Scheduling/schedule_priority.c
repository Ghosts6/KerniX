#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "task.h"
#include "list.h"
#include "schedulers.h"
#include "cpu.h"

// Priority Scheduling

static Task **tasks = NULL;
static int task_count = 0;
static int capacity = 10;
static int *completed = NULL;

void add(char *name, int priority, int burst) {
    if (tasks == NULL) {
        tasks = malloc(sizeof(Task*) * capacity);
        completed = malloc(sizeof(int) * capacity);
    }
    if (task_count >= capacity) {
        capacity *= 2;
        tasks = realloc(tasks, sizeof(Task*) * capacity);
        completed = realloc(completed, sizeof(int) * capacity);
    }

    Task *newTask = malloc(sizeof(Task));
    newTask->name = malloc(strlen(name) + 1);
    strcpy(newTask->name, name);
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->tid = task_count;

    tasks[task_count] = newTask;
    completed[task_count] = 0;
    task_count++;
}

void schedule() {
    int current_time = 0;
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;
    int executed = 0;

    while (executed < task_count) {
        int highest_idx = -1;
        int highest_priority = -1;

        // Find next task with highest priority (unexecuted, earliest tid for ties)
        for (int i = 0; i < task_count; i++) {
            if (!completed[i] && tasks[i]->priority > highest_priority) {
                highest_priority = tasks[i]->priority;
                highest_idx = i;
            }
        }

        if (highest_idx == -1) break;

        int response_time = current_time;
        int execution_time = tasks[highest_idx]->burst;

        run(tasks[highest_idx], execution_time);

        current_time += execution_time;
        int completion_time = current_time;
        int turnaround_time = completion_time;
        int waiting_time = turnaround_time - execution_time;

        total_response += response_time;
        total_waiting += waiting_time;
        total_turnaround += turnaround_time;

        completed[highest_idx] = 1;
        executed++;
    }

    printf("\n");
    printf("Average waiting time = %.2f\n", total_waiting / task_count);
    printf("Average turnaround time = %.2f\n", total_turnaround / task_count);
    printf("Average response time = %.2f\n", total_response / task_count);
}
