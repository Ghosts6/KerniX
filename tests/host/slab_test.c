#include <kernix/slab.h>

#include <stdio.h>
#include <stdint.h>

static int failures;

static void expect_true(const char *name, int cond) {
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", name);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

int main(void) {
    slab_init();
    slab_cache_t *c = slab_create("obj32", 32);
    expect_true("create cache", c != 0);

    void *a = slab_alloc(c);
    void *b = slab_alloc(c);
    expect_true("alloc a", a != 0);
    expect_true("alloc b", b != 0);
    expect_true("distinct", a != b);

    slab_free(c, a);
    void *c2 = slab_alloc(c);
    expect_true("realloc after free", c2 == a);

    slab_free(c, b);
    slab_free(c, c2);
    expect_true("alloc count", slab_alloc_count(c) == 3);
    expect_true("free count", slab_free_count(c) == 3);

    // Fill one slab then allocate into a second page.
    void *ptrs[128];
    uint32_t n = 0;
    for (; n < 128; n++) {
        ptrs[n] = slab_alloc(c);
        if (ptrs[n] == 0) {
            break;
        }
    }
    expect_true("filled some", n > 8);
    for (uint32_t i = 0; i < n; i++) {
        slab_free(c, ptrs[i]);
    }
    expect_true("freed fill", slab_free_count(c) >= slab_alloc_count(c) - 3);

    return failures ? 1 : 0;
}
