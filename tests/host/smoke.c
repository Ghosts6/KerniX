#include <kernix/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Host smoke test: shared headers and constants used by the kernel.

static int failures;

static void expect_eq_u32(const char *name, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got 0x%x want 0x%x\n", name, got, want);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

static void expect_str(const char *name, const char *got, const char *want) {
    if (strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL %s: got '%s' want '%s'\n", name, got, want);
        failures++;
        return;
    }
    printf("ok %s\n", name);
}

int main(void) {
    expect_eq_u32("multiboot magic", MULTIBOOT_BOOTLOADER_MAGIC, 0x2BADB002u);
    expect_str("boot banner", KERNIX_BANNER, "KerniX boot OK");

    if (failures != 0) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("all host smoke tests passed\n");
    return 0;
}
