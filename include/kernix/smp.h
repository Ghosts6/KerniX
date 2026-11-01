#ifndef KERNIX_SMP_H
#define KERNIX_SMP_H

#include <stdint.h>

#define SMP_MAX_CPUS 4

void smp_init(void);
uint32_t smp_cpu_count(void);
uint32_t smp_cpu_id(void);

#endif
