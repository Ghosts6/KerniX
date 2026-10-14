#include <kernix/isr.h>
#include <kernix/idt.h>
#include <kernix/pic.h>
#include <kernix/pit.h>
#include <kernix/kprintf.h>
#include <kernix/qemu.h>
#include <kernix/cpu.h>
#include <kernix/io.h>
#include <kernix/paging.h>
#include <kernix/sched.h>

static const char *exception_name(uint32_t num) {
    static const char *names[] = {
        "divide error",
        "debug",
        "NMI",
        "breakpoint",
        "overflow",
        "bound range",
        "invalid opcode",
        "device not available",
        "double fault",
        "coprocessor segment",
        "invalid TSS",
        "segment not present",
        "stack fault",
        "general protection",
        "page fault",
        "reserved",
        "x87 FPU",
        "alignment check",
        "machine check",
        "SIMD FPU",
    };

    if (num < sizeof(names) / sizeof(names[0])) {
        return names[num];
    }
    return "unknown";
}

void isr_init(void) {
    idt_init();
}

void isr_handler(interrupt_frame_t *frame) {
    kprintf("exception %u (%s) err=0x%x\n", frame->int_no, exception_name(frame->int_no),
            frame->err_code);
    kprintf("  eax=%x ebx=%x ecx=%x edx=%x\n", frame->eax, frame->ebx, frame->ecx, frame->edx);
    kprintf("  esi=%x edi=%x ebp=%x esp=%x\n", frame->esi, frame->edi, frame->ebp, frame->esp);
    kprintf("  eip=%x cs=%x eflags=%x\n", frame->eip, frame->cs, frame->eflags);

    if (frame->int_no == 14) {
        page_fault_dump(frame->err_code, read_cr2());
    }

    // Report the fault cleanly for tests; do not triple-fault.
    qemu_exit(QEMU_EXIT_SUCCESS);
}

void irq_handler(interrupt_frame_t *frame) {
    uint8_t irq = (uint8_t)(frame->int_no - PIC_IRQ_BASE);

    if (frame->int_no == 32) {
        // EOI first: sched_on_tick may switch to a task that never returns here.
        pic_send_eoi(irq);
        pit_on_tick();
        sched_on_tick();
        return;
    }

    if (frame->int_no == 33) {
        (void)inb(0x60); // drain keyboard scancode
    }

    pic_send_eoi(irq);
}
