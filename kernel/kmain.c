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
#include <kernix/paging.h>
#include <kernix/heap.h>
#include <kernix/sched.h>
#include <kernix/sched_selftest.h>
#include <kernix/sync_selftest.h>
#include <kernix/user_selftest.h>

#include <stdbool.h>
#include <stdint.h>

#define PIT_HZ           100
#define TIMER_WAIT_TICKS 10
#define KERNEL_HEAP_SIZE (256u * 1024u)

extern char kernel_start[];
extern char kernel_end[];

static uint8_t kernel_heap[KERNEL_HEAP_SIZE] __attribute__((aligned(16)));

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

// Value of "key=value" in the cmdline, terminated by space/tab/NUL; NULL if absent.
static const char *cmdline_value(const char *cmdline, const char *key) {
    if (cmdline == 0 || key == 0 || *key == '\0') {
        return 0;
    }

    for (const char *p = cmdline; *p; p++) {
        if (p != cmdline && p[-1] != ' ' && p[-1] != '\t') {
            continue;
        }
        const char *a = p;
        const char *b = key;
        while (*a && *b && *a == *b) {
            a++;
            b++;
        }
        if (*b == '\0') {
            return a;
        }
    }
    return 0;
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

static void trigger_page_fault(void) {
    volatile uint32_t *unmapped = (volatile uint32_t *)0xDEADB000u;
    *unmapped = 1;
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
    kprintf("kernel at %x-%x (higher half)\n", (uint32_t)(uintptr_t)kernel_start,
            (uint32_t)(uintptr_t)kernel_end);

    pmm_init(mbi_addr);
    pmm_print_stats();

    paging_init();

    heap_init(kernel_heap, sizeof(kernel_heap));
    void *smoke = kmalloc(64);
    if (smoke == 0) {
        panic("heap smoke alloc failed");
    }
    kfree(smoke);
    kprintf("heap: %u KiB ready\n", KERNEL_HEAP_SIZE / 1024u);

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

    if (cmdline_has(cmdline, "fault=pf")) {
        kprintf("triggering page fault\n");
        trigger_page_fault();
        panic("page fault did not fire");
    }

    const sched_policy_t *policy = &sched_rr;
    const char *policy_arg = cmdline_value(cmdline, "sched=");
    if (policy_arg != 0) {
        policy = sched_policy_find(policy_arg);
        if (policy == 0) {
            panic("unknown sched policy: %s", policy_arg);
        }
    }
    sched_init(policy);
    kprintf("sched: policy=%s quantum=%u ticks\n", policy->name, SCHED_QUANTUM_TICKS);
    if (!sched_selftest()) {
        qemu_exit(QEMU_EXIT_FAILURE);
    }
    if (!sync_selftest()) {
        qemu_exit(QEMU_EXIT_FAILURE);
    }
    if (!user_selftest(mbi_addr)) {
        qemu_exit(QEMU_EXIT_FAILURE);
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
