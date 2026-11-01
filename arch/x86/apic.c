#include <kernix/apic.h>
#include <kernix/cpu.h>
#include <kernix/kprintf.h>

#include <stdint.h>

static int have_apic = -1;

bool apic_present(void) {
    if (have_apic >= 0) {
        return have_apic != 0;
    }

    uint32_t eax, ebx, ecx, edx;
    eax = 1;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(eax));
    have_apic = (edx & (1u << 9)) != 0 ? 1 : 0;
    return have_apic != 0;
}

void apic_init(void) {
    if (!apic_present()) {
        kprintf("apic: not present, keeping PIC\n");
        return;
    }
    // Educational stub: detect LAPIC but leave the 8259 PIC as the IRQ source.
    // Full AP bring-up / APIC timer is left for a later SMP milestone.
    kprintf("apic: LAPIC present at %x (UP mode, PIC retained)\n", LAPIC_DEFAULT_PHYS);
}
