#ifndef KERNIX_SPINLOCK_H
#define KERNIX_SPINLOCK_H

#include <stdint.h>

#ifndef KERNIX_HOST_TEST
#include <kernix/cpu.h>
#endif

typedef struct spinlock {
    volatile uint32_t locked;
} spinlock_t;

#define SPINLOCK_INIT {0}

static inline void spin_lock_init(spinlock_t *lock) {
    lock->locked = 0;
}

// Atomic exchange: returns the previous value at *ptr.
static inline uint32_t atomic_xchg(volatile uint32_t *ptr, uint32_t val) {
#if defined(__i386__) || defined(__x86_64__)
    uint32_t ret;
    __asm__ volatile("xchgl %0, %1" : "=r"(ret), "+m"(*ptr) : "0"(val) : "memory");
    return ret;
#else
    return __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL);
#endif
}

static inline void spin_lock(spinlock_t *lock) {
    while (atomic_xchg(&lock->locked, 1) != 0) {
#if defined(__i386__) || defined(__x86_64__)
        __asm__ volatile("pause");
#else
        __asm__ volatile("" ::: "memory");
#endif
    }
}

static inline void spin_unlock(spinlock_t *lock) {
    __atomic_store_n(&lock->locked, 0, __ATOMIC_RELEASE);
}

// Disable interrupts, then take the lock. Pair with spin_unlock_irqrestore.
static inline uint32_t spin_lock_irqsave(spinlock_t *lock) {
#ifdef KERNIX_HOST_TEST
    spin_lock(lock);
    return 0;
#else
    uint32_t flags = irq_save();
    spin_lock(lock);
    return flags;
#endif
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flags) {
    spin_unlock(lock);
#ifdef KERNIX_HOST_TEST
    (void)flags;
#else
    irq_restore(flags);
#endif
}

#endif
