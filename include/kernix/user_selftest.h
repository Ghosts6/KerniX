#ifndef KERNIX_USER_SELFTEST_H
#define KERNIX_USER_SELFTEST_H

#include <stdbool.h>
#include <stdint.h>

// Spawn Multiboot module 0 (or built-in hello), wait for exit, check serial path.
bool user_selftest(uint32_t mbi_addr);

#endif
