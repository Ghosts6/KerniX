#ifndef KERNIX_SLAB_H
#define KERNIX_SLAB_H

#include <stddef.h>
#include <stdint.h>

#define SLAB_MAX_CACHES 8

typedef struct slab_cache {
    const char *name;
    size_t obj_size;
    size_t objects_per_slab;
    void *partial; // linked list of slabs with free objects
    uint32_t allocs;
    uint32_t frees;
    int in_use;
} slab_cache_t;

void slab_init(void);
slab_cache_t *slab_create(const char *name, size_t obj_size);
void *slab_alloc(slab_cache_t *cache);
void slab_free(slab_cache_t *cache, void *obj);

// Host/kernel test helpers.
uint32_t slab_alloc_count(const slab_cache_t *cache);
uint32_t slab_free_count(const slab_cache_t *cache);

#endif
