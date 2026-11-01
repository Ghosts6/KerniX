#ifndef KERNIX_PMM_H
#define KERNIX_PMM_H

#include <stdint.h>
#include <stdbool.h>

#define PMM_FRAME_SIZE 4096u

void pmm_init(uint32_t mbi_addr);
void *pmm_alloc_frame(void);
void pmm_free_frame(void *frame);
void pmm_ref_frame(void *frame);
void pmm_unref_frame(void *frame);
uint32_t pmm_frame_refcount(void *frame);
uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);
void pmm_print_stats(void);

// Host-test helpers (also usable by the kernel).
void pmm_test_reset(uint32_t frame_count);
void pmm_test_mark_used(uint32_t frame);
void pmm_test_mark_free(uint32_t frame);
bool pmm_test_is_used(uint32_t frame);

#endif
