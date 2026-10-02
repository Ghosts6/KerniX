#include <kernix/kernel.h>
#include <kernix/kprintf.h>
#include <kernix/qemu.h>
#include <kernix/serial.h>
#include <kernix/vga.h>
#include <kernix/gdt.h>
#include <kernix/isr.h>
#include <kernix/pic.h>
#include <kernix/pit.h>
#include <kernix/cpu.h>
#include <kernix/multiboot.h>
#include <kernix/pmm.h>

#include <stdbool.h>

#define PIT_HZ           100
#define TIMER_WAIT_TICKS 10

static bool cmdline_has(const char *cmdline, const char *token) {
    if (cmdline == 0 || token == 0 || *token == '\0') {
        return false;
    }

    for (const char *p = cmdline; *p; p++) {
        const char *a = p;
        const char *b = token;
        while (*a && *b && *a == *b) {
            a++;
            b++;
        }
        if (*b == '\0' && (*a == '\0' || *a == ' ' || *a == '\t')) {
            if (p == cmdline || p[-1] == ' ' || p[-1] == '\t') {
                return true;
            }
        }
    }
    return false;
}

static void trigger_divide_error(void) {
    __asm__ volatile(
        "movl $1, %%eax\n\t"
        "xorl %%edx, %%edx\n\t"
        "xorl %%ecx, %%ecx\n\t"
        "divl %%ecx"
        :
        :
        : "eax", "edx", "ecx");
}

static void trigger_breakpoint(void) {
    __asm__ volatile("int3");
}

void kmain(uint32_t magic, uint32_t mbi_addr) {
    serial_init();
    vga_init();

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("bad multiboot magic: 0x%x", magic);
    }

    const char *cmdline = multiboot_cmdline(mbi_addr);

    gdt_init();
    isr_init();
    pic_remap(PIC_IRQ_BASE, PIC_IRQ_BASE + 8);
    // Unmask IRQ0 (PIT) and IRQ1 (keyboard); mask the rest.
    pic_set_mask(0xFFFC);
    pit_init(PIT_HZ);
    sti();

    kprintf("%s\n", KERNIX_BANNER);

    pmm_init(mbi_addr);
    pmm_print_stats();

    if (cmdline_has(cmdline, "fault=de")) {
        kprintf("triggering divide error\n");
        trigger_divide_error();
        panic("divide error did not fire");
    }

    if (cmdline_has(cmdline, "fault=bp")) {
        kprintf("triggering breakpoint\n");
        trigger_breakpoint();
        panic("breakpoint did not fire");
    }

    while (pit_ticks() < TIMER_WAIT_TICKS) {
        hlt();
    }

    kprintf("timer ok ticks=%u\n", pit_ticks());
    qemu_exit(QEMU_EXIT_SUCCESS);

    cli();
    for (;;) {
        hlt();
    }
}
