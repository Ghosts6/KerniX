#include <kernix/multiboot.h>

#include <stdint.h>

const multiboot_info_t *multiboot_info(uint32_t mbi_addr) {
    if (mbi_addr == 0) {
        return 0;
    }
    return (const multiboot_info_t *)(uintptr_t)mbi_addr;
}

const char *multiboot_cmdline(uint32_t mbi_addr) {
    const multiboot_info_t *mbi = multiboot_info(mbi_addr);
    if (mbi == 0) {
        return "";
    }
    if ((mbi->flags & MULTIBOOT_INFO_CMDLINE) == 0 || mbi->cmdline == 0) {
        return "";
    }
    return (const char *)(uintptr_t)mbi->cmdline;
}
