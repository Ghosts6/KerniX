#include <kernix/sync_selftest.h>
#include <kernix/cpu.h>
#include <kernix/kprintf.h>
#include <kernix/mutex.h>
#include <kernix/sched.h>
#include <kernix/semaphore.h>

#include <stdint.h>

#define INC_THREADS 4
#define INC_PER_THREAD 200

#define BUF_CAP 8
#define PC_PRODUCERS 2
#define PC_CONSUMERS 2
#define PC_EACH 40

static mutex_t counter_mu;
static volatile uint32_t shared_counter;

static void incrementer(void *arg) {
    (void)arg;
    for (uint32_t i = 0; i < INC_PER_THREAD; i++) {
        mutex_lock(&counter_mu);
        shared_counter++;
        mutex_unlock(&counter_mu);
        if ((i & 7u) == 0) {
            sched_yield();
        }
    }
}

static bool run_mutex_test(void) {
    shared_counter = 0;
    mutex_init(&counter_mu);

    uint32_t flags = irq_save();
    for (int i = 0; i < INC_THREADS; i++) {
        if (task_create("inc", incrementer, 0, 5, 0) == 0) {
            sched_wait_all();
            irq_restore(flags);
            sched_release();
            kprintf("sync mutex FAIL task_create\n");
            return false;
        }
    }
    sched_wait_all();
    irq_restore(flags);
    sched_release();

    uint32_t want = (uint32_t)INC_THREADS * INC_PER_THREAD;
    if (shared_counter != want) {
        kprintf("sync mutex FAIL counter=%u want=%u\n", shared_counter, want);
        return false;
    }
    kprintf("sync mutex ok counter=%u\n", shared_counter);
    return true;
}

static semaphore_t sem_empty;
static semaphore_t sem_full;
static mutex_t buf_mu;
static int buffer[BUF_CAP];
static uint32_t buf_head;
static uint32_t buf_tail;
static uint32_t buf_count;
static volatile uint32_t produced_sum;
static volatile uint32_t consumed_sum;

static void producer(void *arg) {
    int base = (int)(uintptr_t)arg;
    for (uint32_t i = 0; i < PC_EACH; i++) {
        int item = base + (int)i;
        sem_wait(&sem_empty);
        mutex_lock(&buf_mu);
        buffer[buf_tail] = item;
        buf_tail = (buf_tail + 1u) % BUF_CAP;
        buf_count++;
        produced_sum += (uint32_t)item;
        mutex_unlock(&buf_mu);
        sem_post(&sem_full);
        if ((i & 3u) == 0) {
            sched_yield();
        }
    }
}

static void consumer(void *arg) {
    (void)arg;
    for (uint32_t i = 0; i < PC_EACH; i++) {
        sem_wait(&sem_full);
        mutex_lock(&buf_mu);
        int item = buffer[buf_head];
        buf_head = (buf_head + 1u) % BUF_CAP;
        buf_count--;
        consumed_sum += (uint32_t)item;
        mutex_unlock(&buf_mu);
        sem_post(&sem_empty);
        if ((i & 3u) == 0) {
            sched_yield();
        }
    }
}

static bool run_producer_consumer(void) {
    sem_init(&sem_empty, BUF_CAP);
    sem_init(&sem_full, 0);
    mutex_init(&buf_mu);
    buf_head = buf_tail = buf_count = 0;
    produced_sum = consumed_sum = 0;

    uint32_t flags = irq_save();
    for (int i = 0; i < PC_PRODUCERS; i++) {
        if (task_create("prod", producer, (void *)(uintptr_t)(i * 1000), 5, 0) == 0) {
            sched_wait_all();
            irq_restore(flags);
            sched_release();
            kprintf("sync pc FAIL producer create\n");
            return false;
        }
    }
    for (int i = 0; i < PC_CONSUMERS; i++) {
        if (task_create("cons", consumer, 0, 5, 0) == 0) {
            sched_wait_all();
            irq_restore(flags);
            sched_release();
            kprintf("sync pc FAIL consumer create\n");
            return false;
        }
    }
    sched_wait_all();
    irq_restore(flags);
    sched_release();

    // sched_wait_all already means every producer/consumer exited.
    if (buf_count != 0 || produced_sum != consumed_sum) {
        kprintf("sync pc FAIL buf=%u produced=%u consumed=%u\n", buf_count, produced_sum,
                consumed_sum);
        return false;
    }
    kprintf("sync pc ok items=%u sum=%u\n", PC_PRODUCERS * PC_EACH, produced_sum);
    return true;
}

bool sync_selftest(void) {
    bool ok = run_mutex_test();
    ok = run_producer_consumer() && ok;
    return ok;
}
