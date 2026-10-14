#ifndef KERNIX_SCHED_DEMO_H
#define KERNIX_SCHED_DEMO_H

#include <stdbool.h>

// Run worker threads under the active policy and compare against the simulator.
bool sched_selftest(void);

#endif
