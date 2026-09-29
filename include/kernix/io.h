#ifndef KERNIX_IO_H
#define KERNIX_IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) {
    // Port 0x80 is a spare POST port; a write delays ~1 us on real hardware.
    outb(0x80, 0);
}

#endif
