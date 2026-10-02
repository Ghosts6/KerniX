#include <kernix/multiboot.h>

const char *multiboot_cmdline(uint32_t mbi_addr) {
    if (mbi_addr == 0) {
        return "";
    }

    const multiboot_info_t *mbi = (const multiboot_info_t *)mbi_addr;
    if ((mbi->flags & MULTIBOOT_INFO_CMDLINE) == 0 || mbi->cmdline == 0) {
        return "";
    }

    return (const char *)mbi->cmdline;
}
