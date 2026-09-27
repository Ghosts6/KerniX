#include <kernix/kernel.h>
#include <stdio.h>
#include <stdlib.h>

// Stage 0 host smoke test: verifies the host test harness builds and runs.

static int failures;

static void expect_eq_u32(const char *name, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got 0x%x want 0x%x\n", name, got, want);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

int main(void) {
    expect_eq_u32("multiboot magic", MULTIBOOT_BOOTLOADER_MAGIC, 0x2BADB002u);

    if (failures != 0) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("all host smoke tests passed\n");
    return 0;
}
