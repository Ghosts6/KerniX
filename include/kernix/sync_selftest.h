#ifndef KERNIX_SYNC_SELFTEST_H
#define KERNIX_SYNC_SELFTEST_H

#include <stdbool.h>

// Mutex lost-update test + bounded-buffer producer/consumer.
bool sync_selftest(void);

#endif
