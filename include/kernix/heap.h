#ifndef KERNIX_HEAP_H
#define KERNIX_HEAP_H

#include <stddef.h>
#include <stdint.h>

void heap_init(void *start, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);

// Host-test helpers.
size_t heap_used_bytes(void);
size_t heap_free_bytes(void);

#endif
