#ifndef KERNIX_APIC_H
#define KERNIX_APIC_H

#include <stdbool.h>
#include <stdint.h>

// Local APIC MMIO (default ISA legacy address). Stage 8: detect only.
#define LAPIC_DEFAULT_PHYS 0xFEE00000u

bool apic_present(void);
void apic_init(void);

#endif
