#include <kernix/user_selftest.h>
#include <kernix/cpu.h>
#include <kernix/kprintf.h>
#include <kernix/proc.h>
#include <kernix/sched.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Minimal flat binary: null-deref then hang (assembled from user/fault.asm).
// ORG 0x08048000: xor eax,eax; mov [eax],1; jmp $
static const uint8_t fault_image[] = {
    0x31, 0xC0,             // xor eax, eax
    0xC7, 0x00, 0x01, 0x00, 0x00, 0x00, // mov dword [eax], 1
    0xEB, 0xFE,             // jmp $
};

static bool run_hello(uint32_t mbi_addr) {
    uint32_t flags = irq_save();
    task_t *t = user_spawn_module(mbi_addr, "hello");
    if (t == 0) {
        irq_restore(flags);
        kprintf("user hello FAIL no module\n");
        return false;
    }
    sched_wait_all();
    irq_restore(flags);
    sched_release();
    kprintf("user hello ok pid=%u\n", t->id);
    return true;
}

static bool run_fault(void) {
    uint32_t flags = irq_save();
    task_t *t = user_spawn("ufault", fault_image, sizeof(fault_image), 0);
    if (t == 0) {
        irq_restore(flags);
        kprintf("user fault FAIL spawn\n");
        return false;
    }
    uint32_t pid = t->id;
    sched_wait_all();
    irq_restore(flags);
    sched_release();
    kprintf("user fault ok pid=%u\n", pid);
    return true;
}

bool user_selftest(uint32_t mbi_addr) {
    bool ok = run_hello(mbi_addr);
    ok = run_fault() && ok;
    return ok;
}
