#include <kernix/spinlock.h>

#include <stdio.h>
#include <stdint.h>

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
    spinlock_t lock = SPINLOCK_INIT;
    expect_eq_u32("spinlock starts free", lock.locked, 0);

    expect_eq_u32("xchg claims free lock", atomic_xchg(&lock.locked, 1), 0);
    expect_eq_u32("lock now held", lock.locked, 1);
    expect_eq_u32("xchg sees held", atomic_xchg(&lock.locked, 1), 1);

    spin_unlock(&lock);
    expect_eq_u32("unlock clears", lock.locked, 0);

    spin_lock(&lock);
    expect_eq_u32("spin_lock acquires", lock.locked, 1);
    spin_unlock(&lock);

    uint32_t flags = spin_lock_irqsave(&lock);
    expect_eq_u32("irqsave acquires", lock.locked, 1);
    spin_unlock_irqrestore(&lock, flags);
    expect_eq_u32("irqrestore releases", lock.locked, 0);

    // Contended-style sequence without spinning forever: trylock via xchg.
    spin_lock_init(&lock);
    spin_lock(&lock);
    expect_true("second xchg fails while held", atomic_xchg(&lock.locked, 1) == 1);
    spin_unlock(&lock);
    expect_true("xchg succeeds after unlock", atomic_xchg(&lock.locked, 1) == 0);
    spin_unlock(&lock);

    return failures ? 1 : 0;
}
