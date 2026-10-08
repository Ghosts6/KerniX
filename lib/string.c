#include <kernix/string.h>

#include <stdint.h>

// GCC -O2 can rewrite these loops into calls to the same libc builtins
// (memset -> memset, strlen -> strlen), which becomes infinite recursion when
// this file provides those symbols. Disable that transform for GCC; Clang does
// not apply the same rewrite.
#if defined(__GNUC__) && !defined(__clang__)
#define KERNIX_NO_BUILTIN_LOOP \
    __attribute__((optimize("no-tree-loop-distribute-patterns")))
#else
#define KERNIX_NO_BUILTIN_LOOP
#endif

KERNIX_NO_BUILTIN_LOOP
void *memset(void *dst, int value, size_t n) {
    uint8_t *p = dst;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)value;
    }
    return dst;
}

KERNIX_NO_BUILTIN_LOOP
void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

KERNIX_NO_BUILTIN_LOOP
size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}
