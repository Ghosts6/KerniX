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

const multiboot_module_t *multiboot_modules(uint32_t mbi_addr, uint32_t *count) {
    if (count != 0) {
        *count = 0;
    }
    const multiboot_info_t *mbi = multiboot_info(mbi_addr);
    if (mbi == 0) {
        return 0;
    }
    if ((mbi->flags & MULTIBOOT_INFO_MODS) == 0 || mbi->mods_count == 0 || mbi->mods_addr == 0) {
        return 0;
    }
    if (count != 0) {
        *count = mbi->mods_count;
    }
    return (const multiboot_module_t *)(uintptr_t)mbi->mods_addr;
}
