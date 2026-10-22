#include <kernix/syscall.h>
#include <kernix/idt.h>
#include <kernix/gdt.h>
#include <kernix/sched.h>
#include <kernix/serial.h>
#include <kernix/paging.h>
#include <kernix/kprintf.h>

#include <stdbool.h>
#include <stdint.h>

extern void isr128(void);

static bool user_ptr_ok(uint32_t ptr, uint32_t len) {
    if (len == 0) {
        return true;
    }
    if (ptr < USER_CODE_BASE) {
        return false;
    }
    uint32_t end = ptr + len;
    if (end < ptr || end > USER_STACK_TOP) {
        return false;
    }
    return true;
}

static int32_t sys_write(uint32_t fd, uint32_t buf, uint32_t len) {
    if (fd != 1) {
        return -1;
    }
    if (!user_ptr_ok(buf, len)) {
        return -1;
    }
    const char *p = (const char *)(uintptr_t)buf;
    for (uint32_t i = 0; i < len; i++) {
        serial_putc(p[i]);
    }
    return (int32_t)len;
}

void syscall_handler(interrupt_frame_t *frame) {
    uint32_t num = frame->eax;
    int32_t ret = -1;

    switch (num) {
    case SYS_EXIT:
        task_exit();
        break;
    case SYS_WRITE:
        ret = sys_write(frame->ebx, frame->ecx, frame->edx);
        break;
    case SYS_YIELD:
        sched_yield();
        ret = 0;
        break;
    case SYS_GETPID: {
        task_t *t = sched_current();
        ret = t != 0 ? (int32_t)t->id : -1;
        break;
    }
    default:
        kprintf("syscall: unknown %u from pid=%u\n", num,
                sched_current() != 0 ? sched_current()->id : 0);
        ret = -1;
        break;
    }

    frame->eax = (uint32_t)ret;
}

void syscall_init(void) {
    // Present, DPL 3, 32-bit interrupt gate so user code can int 0x80.
    idt_set_gate(SYSCALL_VECTOR, (uint32_t)isr128, GDT_KERNEL_CODE, 0xEE);
}
