#include <kernix/vga.h>

#include <stddef.h>

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_ATTR   0x0F // white on black

static volatile uint16_t *const vga = (volatile uint16_t *)0xB8000;
static size_t row;
static size_t col;

static uint16_t vga_entry(char c) {
    return (uint16_t)c | ((uint16_t)VGA_ATTR << 8);
}

static void vga_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ');
    }
}

void vga_clear(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = vga_entry(' ');
    }
    row = 0;
    col = 0;
}

void vga_init(void) {
    vga_clear();
}

void vga_putc(char c) {
    if (c == '\n') {
        col = 0;
        row++;
    } else if (c == '\r') {
        col = 0;
    } else if (c == '\t') {
        col = (col + 4) & ~(size_t)3;
        if (col >= VGA_WIDTH) {
            col = 0;
            row++;
        }
    } else {
        vga[row * VGA_WIDTH + col] = vga_entry(c);
        col++;
        if (col >= VGA_WIDTH) {
            col = 0;
            row++;
        }
    }

    if (row >= VGA_HEIGHT) {
        vga_scroll();
        row = VGA_HEIGHT - 1;
    }
}

void vga_write(const char *s) {
    while (*s) {
        vga_putc(*s++);
    }
}
