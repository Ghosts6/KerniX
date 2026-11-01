#ifndef KERNIX_SYSCALL_H
#define KERNIX_SYSCALL_H

#include <kernix/isr.h>

#include <stdint.h>

#define SYSCALL_VECTOR 0x80

#define SYS_EXIT   0
#define SYS_WRITE  1
#define SYS_YIELD  2
#define SYS_GETPID 3
#define SYS_FORK   4

void syscall_init(void);
void syscall_handler(interrupt_frame_t *frame);

#endif
