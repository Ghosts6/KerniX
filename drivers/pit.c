#include <kernix/pit.h>
#include <kernix/io.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_INPUT_HZ 1193182u

static volatile uint32_t ticks;

void pit_init(uint32_t frequency_hz) {
    if (frequency_hz == 0) {
        frequency_hz = 100;
    }

    uint32_t divisor = PIT_INPUT_HZ / frequency_hz;
    if (divisor == 0) {
        divisor = 1;
    }
    if (divisor > 65535u) {
        divisor = 65535u;
    }

    // Channel 0, lobyte/hibyte, mode 3 (square wave), binary.
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    ticks = 0;
}

void pit_on_tick(void) {
    ticks++;
}

uint32_t pit_ticks(void) {
    return ticks;
}
