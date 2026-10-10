#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "task.h"
#include "list.h"
#include "schedulers.h"
#include "cpu.h"

// Tasks are kept in a dynamic array so input order is preserved
// (list.c's insert() prepends, which would reverse the order).
static Task **tasks = NULL;
static int task_count = 0;
static int capacity = 10;

/**
 * Called by driver.c once per input line. Copies the task into the array,
 * doubling the array's capacity when it is full so any number of tasks works.
 */
void add(char *name, int priority, int burst) {
    if (tasks == NULL) {
        tasks = malloc(sizeof(Task*) * capacity);
    }
    if (task_count >= capacity) {
        capacity *= 2;
        tasks = realloc(tasks, sizeof(Task*) * capacity);
    }

    Task *newTask = malloc(sizeof(Task));
    newTask->name = malloc(strlen(name) + 1);
    strcpy(newTask->name, name);
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->tid = task_count;

    tasks[task_count++] = newTask;
}

/**
 * Runs every task once, start to finish, in array (input) order.
 * Since all tasks arrive at 0 and FCFS is non-preemptive:
 *   response = waiting = start time, turnaround = completion time.
 */
void schedule() {
    int current_time = 0;
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;

    for (int i = 0; i < task_count; i++) {
        int response_time = current_time;
        int execution_time = tasks[i]->burst;

        run(tasks[i], execution_time);

        current_time += execution_time;
        int completion_time = current_time;
        int turnaround_time = completion_time;
        int waiting_time = turnaround_time - execution_time;

        total_response += response_time;
        total_waiting += waiting_time;
        total_turnaround += turnaround_time;
    }

    printf("\n");
    printf("Average waiting time = %.2f\n", total_waiting / task_count);
    printf("Average turnaround time = %.2f\n", total_turnaround / task_count);
    printf("Average response time = %.2f\n", total_response / task_count);
}
