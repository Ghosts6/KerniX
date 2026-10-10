#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "task.h"
#include "list.h"
#include "schedulers.h"
#include "cpu.h"

// Tasks are kept in input order in a dynamic array; completed[i] marks
// tasks[i] as already run so it is skipped by later selections.
static Task **tasks = NULL;
static int task_count = 0;
static int capacity = 10;
static int *completed = NULL;

/**
 * Called by driver.c once per input line. Copies the task into the array,
 * doubling capacity (for both arrays) when full so any number of tasks works.
 */
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

/**
 * Returns the index of the unfinished task with the shortest burst, or -1 if
 * all tasks are done. The strict '<' keeps the first match on ties, which is
 * the earliest task in input order.
 */
static int pickNextTask() {
    int shortest_idx = -1;
    int shortest_burst = INT_MAX;

    for (int i = 0; i < task_count; i++) {
        if (!completed[i] && tasks[i]->burst < shortest_burst) {
            shortest_burst = tasks[i]->burst;
            shortest_idx = i;
        }
    }
    return shortest_idx;
}

/**
 * Runs the task chosen by pickNextTask() to completion until none are left.
 * Since all tasks arrive at 0 and SJF is non-preemptive:
 *   response = waiting = start time, turnaround = completion time.
 */
void schedule() {
    int current_time = 0;
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;
    int executed = 0;

    while (executed < task_count) {
        int shortest_idx = pickNextTask();
        if (shortest_idx == -1) break;

        int response_time = current_time;
        int execution_time = tasks[shortest_idx]->burst;

        run(tasks[shortest_idx], execution_time);

        current_time += execution_time;
        int completion_time = current_time;
        int turnaround_time = completion_time;
        int waiting_time = turnaround_time - execution_time;

        total_response += response_time;
        total_waiting += waiting_time;
        total_turnaround += turnaround_time;

        completed[shortest_idx] = 1;
        executed++;
    }

    printf("\n");
    printf("Average waiting time = %.2f\n", total_waiting / task_count);
    printf("Average turnaround time = %.2f\n", total_turnaround / task_count);
    printf("Average response time = %.2f\n", total_response / task_count);
}
