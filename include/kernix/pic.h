#ifndef KERNIX_PIC_H
#define KERNIX_PIC_H

#include <stdint.h>

#define PIC_IRQ_BASE 32

void pic_remap(uint8_t offset_master, uint8_t offset_slave);
void pic_set_mask(uint16_t mask);
void pic_send_eoi(uint8_t irq);

#endif
