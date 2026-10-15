#include <kernix/heap.h>
#include <kernix/spinlock.h>
#include <kernix/string.h>

#include <stdint.h>

typedef struct heap_block {
    size_t size; // payload bytes
    int free;
    struct heap_block *next;
} heap_block_t;

static heap_block_t *heap_head;
static size_t heap_total;
static size_t heap_used;
static spinlock_t heap_lock = SPINLOCK_INIT;

static size_t align_up(size_t n) {
    const size_t a = sizeof(void *);
    return (n + a - 1u) & ~(a - 1u);
}

void heap_init(void *start, size_t size) {
    if (start == 0 || size < sizeof(heap_block_t) + 8u) {
        heap_head = 0;
        heap_total = 0;
        heap_used = 0;
        return;
    }

    heap_head = (heap_block_t *)start;
    heap_head->size = size - sizeof(heap_block_t);
    heap_head->free = 1;
    heap_head->next = 0;
    heap_total = heap_head->size;
    heap_used = 0;
}

static void split_block(heap_block_t *block, size_t want) {
    size_t remain = block->size - want;
    if (remain <= sizeof(heap_block_t) + 8u) {
        return;
    }

    heap_block_t *rest =
        (heap_block_t *)((uint8_t *)block + sizeof(heap_block_t) + want);
    rest->size = remain - sizeof(heap_block_t);
    rest->free = 1;
    rest->next = block->next;

    block->size = want;
    block->next = rest;
}

void *kmalloc(size_t size) {
    if (heap_head == 0 || size == 0) {
        return 0;
    }

    size = align_up(size);
    uint32_t flags = spin_lock_irqsave(&heap_lock);

    for (heap_block_t *b = heap_head; b != 0; b = b->next) {
        if (!b->free || b->size < size) {
            continue;
        }
        split_block(b, size);
        b->free = 0;
        heap_used += b->size;
        void *ptr = (void *)(b + 1);
        spin_unlock_irqrestore(&heap_lock, flags);
        return ptr;
    }
    spin_unlock_irqrestore(&heap_lock, flags);
    return 0;
}

static void coalesce(void) {
    for (heap_block_t *b = heap_head; b != 0 && b->next != 0;) {
        if (b->free && b->next->free) {
            b->size += sizeof(heap_block_t) + b->next->size;
            b->next = b->next->next;
            continue;
        }
        b = b->next;
    }
}

void kfree(void *ptr) {
    if (ptr == 0 || heap_head == 0) {
        return;
    }

    uint32_t flags = spin_lock_irqsave(&heap_lock);
    heap_block_t *block = ((heap_block_t *)ptr) - 1;
    if (block->free) {
        spin_unlock_irqrestore(&heap_lock, flags);
        return;
    }
    block->free = 1;
    if (heap_used >= block->size) {
        heap_used -= block->size;
    } else {
        heap_used = 0;
    }
    coalesce();
    spin_unlock_irqrestore(&heap_lock, flags);
}

size_t heap_used_bytes(void) {
    uint32_t flags = spin_lock_irqsave(&heap_lock);
    size_t used = heap_used;
    spin_unlock_irqrestore(&heap_lock, flags);
    return used;
}

size_t heap_free_bytes(void) {
    uint32_t flags = spin_lock_irqsave(&heap_lock);
    size_t free_bytes = heap_total < heap_used ? 0 : heap_total - heap_used;
    spin_unlock_irqrestore(&heap_lock, flags);
    return free_bytes;
}
