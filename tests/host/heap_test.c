#include <kernix/heap.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;
static uint8_t arena[64 * 1024];

static void expect_true(const char *name, int cond) {
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", name);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

static void expect_eq_size(const char *name, size_t got, size_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %zu want %zu\n", name, got, want);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

int main(void) {
    heap_init(arena, sizeof(arena));
    expect_true("init free", heap_free_bytes() > 0);
    expect_eq_size("init used", heap_used_bytes(), 0);

    void *a = kmalloc(32);
    void *b = kmalloc(64);
    expect_true("alloc a", a != NULL);
    expect_true("alloc b", b != NULL);
    expect_true("distinct", a != b);
    expect_true("used grew", heap_used_bytes() >= 32 + 64);

    memset(a, 0xA5, 32);
    memset(b, 0x5A, 64);
    expect_true("write a", ((uint8_t *)a)[0] == 0xA5);
    expect_true("write b", ((uint8_t *)b)[0] == 0x5A);

    kfree(a);
    void *c = kmalloc(32);
    expect_true("realloc after free", c != NULL);

    kfree(b);
    kfree(c);
    expect_eq_size("all freed", heap_used_bytes(), 0);

    // Exhaustion: ask for more than the arena.
    void *big = kmalloc(sizeof(arena));
    expect_true("oversize fails", big == NULL);

    void *ptrs[64];
    int n = 0;
    for (; n < 64; n++) {
        ptrs[n] = kmalloc(512);
        if (ptrs[n] == NULL) {
            break;
        }
    }
    expect_true("filled some", n > 0);
    if (n < 64) {
        expect_true("eventually exhausted", kmalloc(512) == NULL);
    } else {
        expect_true("filled all slots", 1);
    }

    for (int i = 0; i < n; i++) {
        kfree(ptrs[i]);
    }
    expect_eq_size("restored", heap_used_bytes(), 0);
    void *again = kmalloc(512);
    expect_true("alloc after restore", again != NULL);
    kfree(again);
    expect_eq_size("final free", heap_used_bytes(), 0);

    if (failures != 0) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("all heap tests passed\n");
    return 0;
}
