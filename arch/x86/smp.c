#include <kernix/smp.h>
#include <kernix/apic.h>
#include <kernix/kprintf.h>

#include <stdint.h>

static uint32_t cpu_count = 1;

void smp_init(void) {
    apic_init();
    cpu_count = 1;
    // Stage 8 bound: no SIPI / AP startup. Structure is ready for per-CPU queues.
    kprintf("smp: %u CPU(s) online (uniprocessor)\n", cpu_count);
}

uint32_t smp_cpu_count(void) {
    return cpu_count;
}

uint32_t smp_cpu_id(void) {
    return 0;
}
