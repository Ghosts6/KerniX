#include <kernix/string.h>

#include <stdint.h>

void *memset(void *dst, int value, size_t n) {
    uint8_t *p = dst;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)value;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}
