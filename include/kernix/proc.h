#ifndef KERNIX_PROC_H
#define KERNIX_PROC_H

#include <kernix/task.h>

#include <stddef.h>
#include <stdint.h>

// Load a flat binary into a new user address space and create a runnable task.
// entry_offset is relative to USER_CODE_BASE (usually 0).
task_t *user_spawn(const char *name, const void *image, size_t size, uint32_t entry_offset);

// Load Multiboot module 0 as a flat user binary (entry at offset 0). NULL if absent.
task_t *user_spawn_module(uint32_t mbi_addr, const char *name);

#endif
