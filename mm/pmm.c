#include <kernix/pmm.h>
#include <kernix/multiboot.h>
#include <kernix/string.h>

#include <stddef.h>
#include <stdint.h>

#ifndef KERNIX_HOST_TEST
#include <kernix/kprintf.h>
#include <kernix/kernel.h>
#include <kernix/paging.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void panic(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    abort();
}

void kprintf(const char *fmt, ...) {
    (void)fmt;
}

char kernel_start[1];
char kernel_end[1];
#endif

// Cap tracked RAM at 512 MiB for a fixed bitmap in .bss.
#define PMM_MAX_PHYS_MB 512u
#define PMM_MAX_FRAMES  ((PMM_MAX_PHYS_MB * 1024u * 1024u) / PMM_FRAME_SIZE)
#define PMM_BITMAP_WORDS ((PMM_MAX_FRAMES + 31u) / 32u)

#ifndef KERNIX_HOST_TEST
extern char kernel_start[];
extern char kernel_end[];
#endif

static uint32_t bitmap[PMM_BITMAP_WORDS];
static uint16_t refcount[PMM_MAX_FRAMES];
static uint32_t total_frames;
static uint32_t used_frames;

static void bitmap_set(uint32_t frame) {
    bitmap[frame / 32u] |= (1u << (frame % 32u));
}

static void bitmap_clear(uint32_t frame) {
    bitmap[frame / 32u] &= ~(1u << (frame % 32u));
}

static bool bitmap_test(uint32_t frame) {
    return (bitmap[frame / 32u] & (1u << (frame % 32u))) != 0;
}

static void mark_region_used(uint64_t addr, uint64_t len) {
    if (len == 0) {
        return;
    }

    uint64_t start = addr;
    uint64_t end = addr + len;

    uint32_t first = (uint32_t)(start / PMM_FRAME_SIZE);
    uint32_t last = (uint32_t)((end + PMM_FRAME_SIZE - 1u) / PMM_FRAME_SIZE);
    if (last > total_frames) {
        last = total_frames;
    }

    for (uint32_t f = first; f < last; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }
}

static void mark_region_free(uint64_t addr, uint64_t len) {
    if (len == 0) {
        return;
    }

    uint64_t start = (addr + PMM_FRAME_SIZE - 1u) & ~((uint64_t)PMM_FRAME_SIZE - 1u);
    uint64_t end = (addr + len) & ~((uint64_t)PMM_FRAME_SIZE - 1u);
    if (end <= start) {
        return;
    }

    uint32_t first = (uint32_t)(start / PMM_FRAME_SIZE);
    uint32_t last = (uint32_t)(end / PMM_FRAME_SIZE);
    if (last > total_frames) {
        last = total_frames;
    }

    for (uint32_t f = first; f < last; f++) {
        if (bitmap_test(f)) {
            bitmap_clear(f);
            used_frames--;
        }
    }
}

void pmm_test_reset(uint32_t frame_count) {
    if (frame_count == 0 || frame_count > PMM_MAX_FRAMES) {
        frame_count = PMM_MAX_FRAMES;
    }
    total_frames = frame_count;
    used_frames = frame_count;
    memset(bitmap, 0xFF, sizeof(bitmap));
}

void pmm_test_mark_used(uint32_t frame) {
    if (frame >= total_frames) {
        return;
    }
    if (!bitmap_test(frame)) {
        bitmap_set(frame);
        used_frames++;
    }
}

void pmm_test_mark_free(uint32_t frame) {
    if (frame >= total_frames) {
        return;
    }
    if (bitmap_test(frame)) {
        bitmap_clear(frame);
        used_frames--;
    }
}

bool pmm_test_is_used(uint32_t frame) {
    if (frame >= total_frames) {
        return true;
    }
    return bitmap_test(frame);
}

void pmm_init(uint32_t mbi_addr) {
    const multiboot_info_t *mbi = multiboot_info(mbi_addr);
    if (mbi == 0) {
        panic("pmm_init: missing multiboot info");
    }

    uint64_t max_addr = 0;

    if ((mbi->flags & MULTIBOOT_INFO_MMAP) != 0 && mbi->mmap_addr != 0 && mbi->mmap_length != 0) {
        uint32_t offset = 0;
        while (offset < mbi->mmap_length) {
            const multiboot_mmap_entry_t *entry =
                (const multiboot_mmap_entry_t *)(uintptr_t)(mbi->mmap_addr + offset);
            uint64_t end = entry->addr + entry->len;
            if (end > max_addr) {
                max_addr = end;
            }
            offset += entry->size + sizeof(entry->size);
        }
    } else if ((mbi->flags & MULTIBOOT_INFO_MEMORY) != 0) {
        max_addr = (1024ull + (uint64_t)mbi->mem_upper) * 1024ull;
    } else {
        panic("pmm_init: no memory map");
    }

    if (max_addr > (uint64_t)PMM_MAX_PHYS_MB * 1024ull * 1024ull) {
        max_addr = (uint64_t)PMM_MAX_PHYS_MB * 1024ull * 1024ull;
    }

    total_frames = (uint32_t)(max_addr / PMM_FRAME_SIZE);
    if (total_frames == 0) {
        panic("pmm_init: no frames");
    }
    if (total_frames > PMM_MAX_FRAMES) {
        total_frames = PMM_MAX_FRAMES;
    }

    used_frames = total_frames;
    memset(bitmap, 0xFF, sizeof(bitmap));

    if ((mbi->flags & MULTIBOOT_INFO_MMAP) != 0 && mbi->mmap_addr != 0) {
        uint32_t offset = 0;
        while (offset < mbi->mmap_length) {
            const multiboot_mmap_entry_t *entry =
                (const multiboot_mmap_entry_t *)(uintptr_t)(mbi->mmap_addr + offset);
            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                mark_region_free(entry->addr, entry->len);
            }
            offset += entry->size + sizeof(entry->size);
        }
    } else {
        mark_region_free(0x100000ull, (uint64_t)mbi->mem_upper * 1024ull);
    }

    mark_region_used(0, 0x100000ull);
#ifndef KERNIX_HOST_TEST
    mark_region_used(virt_to_phys((uint32_t)(uintptr_t)kernel_start),
                     (uint32_t)(uintptr_t)(kernel_end - kernel_start));
#else
    mark_region_used((uint32_t)(uintptr_t)kernel_start,
                     (uint32_t)(uintptr_t)(kernel_end - kernel_start));
#endif
    mark_region_used(mbi_addr, sizeof(multiboot_info_t));
    if ((mbi->flags & MULTIBOOT_INFO_MMAP) != 0 && mbi->mmap_addr != 0) {
        mark_region_used(mbi->mmap_addr, mbi->mmap_length);
    }
    if ((mbi->flags & MULTIBOOT_INFO_CMDLINE) != 0 && mbi->cmdline != 0) {
        mark_region_used(mbi->cmdline, strlen((const char *)(uintptr_t)mbi->cmdline) + 1u);
    }
    if ((mbi->flags & MULTIBOOT_INFO_MODS) != 0 && mbi->mods_count != 0 && mbi->mods_addr != 0) {
        mark_region_used(mbi->mods_addr, mbi->mods_count * sizeof(multiboot_module_t));
        const multiboot_module_t *mods =
            (const multiboot_module_t *)(uintptr_t)mbi->mods_addr;
        for (uint32_t i = 0; i < mbi->mods_count; i++) {
            if (mods[i].mod_end > mods[i].mod_start) {
                mark_region_used(mods[i].mod_start, mods[i].mod_end - mods[i].mod_start);
            }
            if (mods[i].string != 0) {
                mark_region_used(mods[i].string,
                                 strlen((const char *)(uintptr_t)mods[i].string) + 1u);
            }
        }
    }
}

void *pmm_alloc_frame(void) {
    for (uint32_t f = 0; f < total_frames; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
            refcount[f] = 1;
            return (void *)(uintptr_t)(f * PMM_FRAME_SIZE);
        }
    }
    return 0;
}

void pmm_free_frame(void *frame) {
    if (frame == 0) {
        return;
    }

    uint32_t addr = (uint32_t)(uintptr_t)frame;
    if ((addr % PMM_FRAME_SIZE) != 0) {
        panic("pmm_free_frame: unaligned %x", addr);
    }

    uint32_t f = addr / PMM_FRAME_SIZE;
    if (f >= total_frames) {
        panic("pmm_free_frame: out of range %x", addr);
    }
    if (!bitmap_test(f)) {
        panic("pmm_free_frame: double free %x", addr);
    }

    refcount[f] = 0;
    bitmap_clear(f);
    used_frames--;
}

void pmm_ref_frame(void *frame) {
    if (frame == 0) {
        return;
    }
    uint32_t f = (uint32_t)(uintptr_t)frame / PMM_FRAME_SIZE;
    if (f >= total_frames || !bitmap_test(f)) {
        return;
    }
    if (refcount[f] < 0xFFFFu) {
        refcount[f]++;
    }
}

void pmm_unref_frame(void *frame) {
    if (frame == 0) {
        return;
    }
    uint32_t f = (uint32_t)(uintptr_t)frame / PMM_FRAME_SIZE;
    if (f >= total_frames || !bitmap_test(f)) {
        return;
    }
    if (refcount[f] > 1) {
        refcount[f]--;
        return;
    }
    pmm_free_frame(frame);
}

uint32_t pmm_frame_refcount(void *frame) {
    if (frame == 0) {
        return 0;
    }
    uint32_t f = (uint32_t)(uintptr_t)frame / PMM_FRAME_SIZE;
    if (f >= total_frames || !bitmap_test(f)) {
        return 0;
    }
    return refcount[f];
}

uint32_t pmm_total_frames(void) {
    return total_frames;
}

uint32_t pmm_used_frames(void) {
    return used_frames;
}

uint32_t pmm_free_frames(void) {
    return total_frames - used_frames;
}

void pmm_print_stats(void) {
    kprintf("pmm: frames total=%u used=%u free=%u (%u KiB free)\n", pmm_total_frames(),
            pmm_used_frames(), pmm_free_frames(),
            (pmm_free_frames() * PMM_FRAME_SIZE) / 1024u);
}
