#include <kernix/kernel.h>
#include <kernix/kprintf.h>
#include <kernix/qemu.h>

#include <stdarg.h>

void panic(const char *fmt, ...) {
    kputs("\nPANIC: ");

    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);

    kputc('\n');
    qemu_exit(QEMU_EXIT_FAILURE);

    __asm__ volatile("cli");
    for (;;) {
        __asm__ volatile("hlt");
    }
}
