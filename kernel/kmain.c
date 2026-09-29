#include <kernix/kernel.h>
#include <kernix/kprintf.h>
#include <kernix/qemu.h>
#include <kernix/serial.h>
#include <kernix/vga.h>

void kmain(uint32_t magic, uint32_t mbi_addr) {
    (void)mbi_addr;

    serial_init();
    vga_init();

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("bad multiboot magic: 0x%x", magic);
    }

    kprintf("%s\n", KERNIX_BANNER);

    qemu_exit(QEMU_EXIT_SUCCESS);

    __asm__ volatile("cli");
    for (;;) {
        __asm__ volatile("hlt");
    }
}
