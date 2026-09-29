#ifndef KERNIX_KPRINTF_H
#define KERNIX_KPRINTF_H

#include <stdarg.h>

void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);
void kvprintf(const char *fmt, va_list ap);

#endif
