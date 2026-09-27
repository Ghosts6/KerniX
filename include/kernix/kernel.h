#ifndef KERNIX_KERNEL_H
#define KERNIX_KERNEL_H

#include <stdint.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

void kmain(uint32_t magic, uint32_t mbi_addr);

#endif
