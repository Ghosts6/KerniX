#ifndef KERNIX_CPU_H
#define KERNIX_CPU_H

#include <stdint.h>

static inline void cli(void) {
    __asm__ volatile("cli");
}

static inline void sti(void) {
    __asm__ volatile("sti");
}

static inline void hlt(void) {
    __asm__ volatile("hlt");
}

#define EFLAGS_IF 0x200u

// Disable interrupts and return the previous EFLAGS for irq_restore.
static inline uint32_t irq_save(void) {
    uint32_t flags;
    __asm__ volatile("pushf\n\tpop %0\n\tcli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(uint32_t flags) {
    if ((flags & EFLAGS_IF) != 0) {
        sti();
    }
}

static inline uint32_t read_cr0(void) {
    uint32_t value;
    __asm__ volatile("mov %%cr0, %0" : "=r"(value));
    return value;
}

static inline void write_cr0(uint32_t value) {
    __asm__ volatile("mov %0, %%cr0" : : "r"(value) : "memory");
}

static inline uint32_t read_cr2(void) {
    uint32_t value;
    __asm__ volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}

static inline uint32_t read_cr3(void) {
    uint32_t value;
    __asm__ volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

static inline void write_cr3(uint32_t value) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(value) : "memory");
}

static inline uint32_t read_cr4(void) {
    uint32_t value;
    __asm__ volatile("mov %%cr4, %0" : "=r"(value));
    return value;
}

static inline void write_cr4(uint32_t value) {
    __asm__ volatile("mov %0, %%cr4" : : "r"(value) : "memory");
}

#endif
