#ifndef KERNIX_QEMU_H
#define KERNIX_QEMU_H

#include <stdint.h>

// isa-debug-exit at 0xf4: QEMU exits with status (code << 1) | 1.
// code 0 -> exit 1 (success), code 1 -> exit 3 (failure).
#define QEMU_EXIT_SUCCESS 0
#define QEMU_EXIT_FAILURE 1

void qemu_exit(uint8_t code);

#endif
