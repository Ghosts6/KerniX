#include <stdint.h>

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static void serial_putc(char c) {
    if (c == '\n') {
        serial_putc('\r');
    }
    while ((inb(COM1 + 5) & 0x20) == 0) {
    }
    outb(COM1, (uint8_t)c);
}

static void serial_write(const char *s) {
    while (*s) {
        serial_putc(*s++);
    }
}

// isa-debug-exit success (QEMU process status 1).
static void qemu_exit_ok(void) {
    __asm__ volatile("outl %0, %1" : : "a"(0), "Nd"((uint16_t)0xf4));
}

void kmain64(void) {
    serial_init();
    serial_write("KerniX x86_64 long mode OK\n");
    qemu_exit_ok();
    for (;;) {
        __asm__ volatile("hlt");
    }
}
