#include <kernix/cxx.h>

#include <stddef.h>
#include <stdint.h>

extern "C" {
#include <kernix/heap.h>
#include <kernix/kernel.h>
}

extern "C" {

typedef void (*ctor_fn)(void);

extern ctor_fn __init_array_start[];
extern ctor_fn __init_array_end[];

void cxx_init(void) {
    for (ctor_fn *fn = __init_array_start; fn < __init_array_end; fn++) {
        if (*fn) {
            (*fn)();
        }
    }
}

void *__dso_handle = 0;

int __cxa_atexit(void (*/*func*/)(void *), void */*arg*/, void */*dso*/) {
    return 0;
}

void __cxa_pure_virtual(void) {
    panic("pure virtual call");
}

} // extern "C"

void *operator new(size_t size) {
    void *p = kmalloc((size_t)size);
    if (p == 0) {
        panic("operator new: out of memory");
    }
    return p;
}

void *operator new[](size_t size) {
    return operator new(size);
}

void operator delete(void *ptr) noexcept {
    kfree(ptr);
}

void operator delete[](void *ptr) noexcept {
    kfree(ptr);
}

void operator delete(void *ptr, size_t) noexcept {
    kfree(ptr);
}

void operator delete[](void *ptr, size_t) noexcept {
    kfree(ptr);
}
