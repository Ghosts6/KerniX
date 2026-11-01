#include <kernix/slab.h>
#include <kernix/pmm.h>
#include <kernix/spinlock.h>
#include <kernix/string.h>
#include <kernix/paging.h>

#include <stdint.h>

typedef struct slab {
    struct slab *next;
    uint32_t free_count;
    uint32_t bitmap[8]; // up to 256 objects
} slab_t;

static slab_cache_t caches[SLAB_MAX_CACHES];
static spinlock_t slab_lock = SPINLOCK_INIT;

#ifndef KERNIX_HOST_TEST
static void *slab_page_alloc(void) {
    void *frame = pmm_alloc_frame();
    if (frame == 0) {
        return 0;
    }
    return (void *)(uintptr_t)phys_to_virt((uint32_t)(uintptr_t)frame);
}

static void slab_page_free(void *page) {
    pmm_free_frame((void *)(uintptr_t)virt_to_phys((uint32_t)(uintptr_t)page));
}
#else
#include <stdlib.h>
static void *slab_page_alloc(void) {
    return aligned_alloc(PMM_FRAME_SIZE, PMM_FRAME_SIZE);
}
static void slab_page_free(void *page) {
    free(page);
}
#endif

static size_t align_up(size_t n, size_t a) {
    return (n + a - 1u) & ~(a - 1u);
}

static size_t objects_for(size_t obj_size) {
    size_t header = sizeof(slab_t);
    if (obj_size == 0 || header >= PMM_FRAME_SIZE) {
        return 0;
    }
    size_t n = (PMM_FRAME_SIZE - header) / obj_size;
    if (n > 256u) {
        n = 256u;
    }
    return n;
}

void slab_init(void) {
    memset(caches, 0, sizeof(caches));
}

slab_cache_t *slab_create(const char *name, size_t obj_size) {
    if (obj_size == 0) {
        return 0;
    }
    obj_size = align_up(obj_size, sizeof(void *));
    size_t per = objects_for(obj_size);
    if (per == 0) {
        return 0;
    }

    uint32_t flags = spin_lock_irqsave(&slab_lock);
    slab_cache_t *cache = 0;
    for (int i = 0; i < SLAB_MAX_CACHES; i++) {
        if (!caches[i].in_use) {
            cache = &caches[i];
            break;
        }
    }
    if (cache == 0) {
        spin_unlock_irqrestore(&slab_lock, flags);
        return 0;
    }
    cache->name = name;
    cache->obj_size = obj_size;
    cache->objects_per_slab = per;
    cache->partial = 0;
    cache->allocs = 0;
    cache->frees = 0;
    cache->in_use = 1;
    spin_unlock_irqrestore(&slab_lock, flags);
    return cache;
}

static slab_t *new_slab(slab_cache_t *cache) {
    uint8_t *page = (uint8_t *)slab_page_alloc();
    if (page == 0) {
        return 0;
    }
    memset(page, 0, PMM_FRAME_SIZE);
    slab_t *s = (slab_t *)page;
    s->next = 0;
    s->free_count = (uint32_t)cache->objects_per_slab;
    memset(s->bitmap, 0, sizeof(s->bitmap));
    return s;
}

static int bitmap_test(const uint32_t *bm, uint32_t bit) {
    return (bm[bit / 32u] & (1u << (bit % 32u))) != 0;
}

static void bitmap_set(uint32_t *bm, uint32_t bit) {
    bm[bit / 32u] |= (1u << (bit % 32u));
}

static void bitmap_clear(uint32_t *bm, uint32_t bit) {
    bm[bit / 32u] &= ~(1u << (bit % 32u));
}

void *slab_alloc(slab_cache_t *cache) {
    if (cache == 0 || !cache->in_use) {
        return 0;
    }

    uint32_t flags = spin_lock_irqsave(&slab_lock);
    slab_t *s = (slab_t *)cache->partial;
    if (s == 0 || s->free_count == 0) {
        s = new_slab(cache);
        if (s == 0) {
            spin_unlock_irqrestore(&slab_lock, flags);
            return 0;
        }
        s->next = (slab_t *)cache->partial;
        cache->partial = s;
    }

    uint32_t idx = 0;
    for (; idx < cache->objects_per_slab; idx++) {
        if (!bitmap_test(s->bitmap, idx)) {
            break;
        }
    }
    if (idx >= cache->objects_per_slab) {
        spin_unlock_irqrestore(&slab_lock, flags);
        return 0;
    }

    bitmap_set(s->bitmap, idx);
    s->free_count--;
    cache->allocs++;
    uint8_t *base = (uint8_t *)s + sizeof(slab_t);
    void *obj = base + idx * cache->obj_size;
    spin_unlock_irqrestore(&slab_lock, flags);
    return obj;
}

void slab_free(slab_cache_t *cache, void *obj) {
    if (cache == 0 || obj == 0 || !cache->in_use) {
        return;
    }

    uint32_t flags = spin_lock_irqsave(&slab_lock);
    uintptr_t page = (uintptr_t)obj & ~((uintptr_t)PMM_FRAME_SIZE - 1u);
    slab_t *s = (slab_t *)page;
    uint8_t *base = (uint8_t *)s + sizeof(slab_t);
    uintptr_t off = (uintptr_t)obj - (uintptr_t)base;
    if (off % cache->obj_size != 0) {
        spin_unlock_irqrestore(&slab_lock, flags);
        return;
    }
    uint32_t idx = (uint32_t)(off / cache->obj_size);
    if (idx >= cache->objects_per_slab || !bitmap_test(s->bitmap, idx)) {
        spin_unlock_irqrestore(&slab_lock, flags);
        return;
    }
    bitmap_clear(s->bitmap, idx);
    s->free_count++;
    cache->frees++;

    if (s->free_count == cache->objects_per_slab) {
        // Drop empty slab from the partial list and free the page.
        slab_t **pp = (slab_t **)&cache->partial;
        while (*pp != 0) {
            if (*pp == s) {
                *pp = s->next;
                break;
            }
            pp = &(*pp)->next;
        }
        slab_page_free(s);
    }
    spin_unlock_irqrestore(&slab_lock, flags);
}

uint32_t slab_alloc_count(const slab_cache_t *cache) {
    return cache != 0 ? cache->allocs : 0;
}

uint32_t slab_free_count(const slab_cache_t *cache) {
    return cache != 0 ? cache->frees : 0;
}
