#include <stddef.h>
#include <stdint.h>

extern "C" {
#include <kernix/kprintf.h>
#include <kernix/heap.h>
}

namespace {

class Counter {
public:
    explicit Counter(uint32_t start) : value_(start) {
        kprintf("cxx: Counter ctor value=%u\n", value_);
    }

    uint32_t bump() {
        return ++value_;
    }

    uint32_t value() const {
        return value_;
    }

private:
    uint32_t value_;
};

static Counter g_counter(41);

} // namespace

extern "C" bool cxx_smoke_test(void) {
    Counter *heap = new Counter(100);
    if (heap == 0) {
        kprintf("cxx smoke FAIL new\n");
        return false;
    }
    uint32_t a = g_counter.bump();
    uint32_t b = heap->bump();
    delete heap;

    if (a != 42 || b != 101) {
        kprintf("cxx smoke FAIL a=%u b=%u\n", a, b);
        return false;
    }
    kprintf("cxx smoke ok\n");
    return true;
}
