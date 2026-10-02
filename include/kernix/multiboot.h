#ifndef KERNIX_MULTIBOOT_H
#define KERNIX_MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT_INFO_CMDLINE (1u << 2)

typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
} multiboot_info_t;

const char *multiboot_cmdline(uint32_t mbi_addr);

#endif
