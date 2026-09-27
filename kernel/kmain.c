#include <kernix/kernel.h>

// Kernel entry, called from arch/x86/boot/boot.asm. Subsystem init goes here as stages land.
void kmain(uint32_t magic, uint32_t mbi_addr) {
    (void)magic;
    (void)mbi_addr;
}
