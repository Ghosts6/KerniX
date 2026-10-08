#include <kernix/pmm.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int failures;

static void expect_true(const char *name, int cond) {
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", name);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

static void expect_eq_u32(const char *name, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %u want %u\n", name, got, want);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

int main(void) {
    pmm_test_reset(32);
    expect_eq_u32("initial used", pmm_used_frames(), 32);
    expect_eq_u32("initial free", pmm_free_frames(), 0);

    for (uint32_t f = 8; f < 16; f++) {
        pmm_test_mark_free(f);
    }
    expect_eq_u32("after mark free", pmm_free_frames(), 8);

    void *a = pmm_alloc_frame();
    void *b = pmm_alloc_frame();
    expect_true("alloc a", a != NULL);
    expect_true("alloc b", b != NULL);
    expect_true("distinct frames", a != b);
    expect_eq_u32("free after two allocs", pmm_free_frames(), 6);

    uint32_t fa = (uint32_t)(uintptr_t)a / PMM_FRAME_SIZE;
    uint32_t fb = (uint32_t)(uintptr_t)b / PMM_FRAME_SIZE;
    expect_true("a in free window", fa >= 8 && fa < 16);
    expect_true("b in free window", fb >= 8 && fb < 16);
    expect_true("a marked used", pmm_test_is_used(fa));
    expect_true("b marked used", pmm_test_is_used(fb));

    pmm_free_frame(a);
    pmm_free_frame(b);
    expect_eq_u32("free after free", pmm_free_frames(), 8);
    expect_true("a free again", !pmm_test_is_used(fa));
    expect_true("b free again", !pmm_test_is_used(fb));

    // Exhaust the free window.
    void *ptrs[8];
    for (int i = 0; i < 8; i++) {
        ptrs[i] = pmm_alloc_frame();
        expect_true("exhaust alloc", ptrs[i] != NULL);
    }
    expect_true("fully exhausted", pmm_alloc_frame() == NULL);
    expect_eq_u32("no free left", pmm_free_frames(), 0);

    for (int i = 0; i < 8; i++) {
        pmm_free_frame(ptrs[i]);
    }
    expect_eq_u32("restored free", pmm_free_frames(), 8);

    if (failures != 0) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("all pmm tests passed\n");
    return 0;
}
