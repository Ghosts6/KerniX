#ifndef KERNIX_GDT_H
#define KERNIX_GDT_H

#include <stdint.h>

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x1B // 0x18 | RPL3
#define GDT_USER_DATA   0x23 // 0x20 | RPL3
#define GDT_TSS         0x28

void gdt_init(void);

// Ring-0 stack used on privilege transitions (syscalls / IRQs from user).
void gdt_set_kernel_stack(uint32_t esp0);

#endif
