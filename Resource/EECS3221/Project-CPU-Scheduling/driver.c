#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"
#include "list.h"
#include "schedulers.h"

#define SIZE    100

int main(int argc, char *argv[])
{
    FILE *in;
    char *line;
    char *temp;
    char task[SIZE];

    char *name;
    char *priority_str;
    char *burst_str;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <schedule file>\n", argv[0]);
        return 1;
    }

    in = fopen(argv[1],"r");
    if (in == NULL) {
        perror(argv[1]);
        return 1;
    }
    
    while (fgets(task,SIZE,in) != NULL) {
        // keep the strdup() pointer: strsep() moves temp, so free(line) later
        line = strdup(task);
        temp = line;
        name = strsep(&temp,",");
        priority_str = strsep(&temp,",");
        burst_str = strsep(&temp,",");

        // skip blank or incomplete lines (e.g. a trailing empty line)
        if (priority_str == NULL || burst_str == NULL) {
            free(line);
            continue;
        }

        // add the task to the scheduler's list of tasks
        add(name,atoi(priority_str),atoi(burst_str));

        free(line);
    }

    fclose(in);

    // invoke the scheduler
    schedule();

    return 0;
}
