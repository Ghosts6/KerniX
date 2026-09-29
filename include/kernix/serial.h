#ifndef KERNIX_SERIAL_H
#define KERNIX_SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);

#endif
