#include <kernix/kprintf.h>
#include <kernix/serial.h>
#include <kernix/vga.h>

#include <stdint.h>

void kputc(char c) {
    serial_putc(c);
    vga_putc(c);
}

void kputs(const char *s) {
    while (*s) {
        kputc(*s++);
    }
}

static void print_uint(uint32_t value, unsigned base) {
    char buf[32];
    unsigned i = 0;

    if (base < 2 || base > 16) {
        return;
    }

    do {
        unsigned digit = value % base;
        buf[i++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
        value /= base;
    } while (value != 0);

    while (i > 0) {
        kputc(buf[--i]);
    }
}

static void print_int(int32_t value) {
    if (value < 0) {
        kputc('-');
        // Avoid overflow on INT_MIN by casting through unsigned.
        print_uint((uint32_t)(-(value + 1)) + 1u, 10);
        return;
    }
    print_uint((uint32_t)value, 10);
}

void kvprintf(const char *fmt, va_list ap) {
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            kputc(*p);
            continue;
        }

        p++;
        switch (*p) {
            case '%':
                kputc('%');
                break;
            case 'c':
                kputc((char)va_arg(ap, int));
                break;
            case 's': {
                const char *s = va_arg(ap, const char *);
                kputs(s ? s : "(null)");
                break;
            }
            case 'd':
            case 'i':
                print_int(va_arg(ap, int));
                break;
            case 'u':
                print_uint(va_arg(ap, unsigned), 10);
                break;
            case 'x':
                print_uint(va_arg(ap, unsigned), 16);
                break;
            case 'p': {
                kputs("0x");
                print_uint((uint32_t)va_arg(ap, void *), 16);
                break;
            }
            case '\0':
                return;
            default:
                kputc('%');
                kputc(*p);
                break;
        }
    }
}

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
}
