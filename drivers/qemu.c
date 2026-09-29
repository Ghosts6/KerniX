#include <kernix/qemu.h>
#include <kernix/io.h>

#define ISA_DEBUG_EXIT_PORT 0xF4

void qemu_exit(uint8_t code) {
    outb(ISA_DEBUG_EXIT_PORT, code);
}
