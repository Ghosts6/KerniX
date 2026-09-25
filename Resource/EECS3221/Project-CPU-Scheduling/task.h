#ifndef TASK_H
#define TASK_H

// Representation of a task in the system.

typedef struct task {
    char *name;
    int tid;
    int priority;
    int burst;
} Task;

#endif
