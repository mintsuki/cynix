#include <stdbool.h>
#include <lib/spinlock.k.h>
#include <lib/panic.k.h>
#include <sys/cpu.k.h>

bool spinlock_test_acq(struct spinlock *spinlock) {
    if (interrupt_state() != false) {
        panic(NULL, "Attempted to acquire spinlock with interrupts enabled");
    }

    return __sync_bool_compare_and_swap(&spinlock->spinlock, false, true);
}

void spinlock_acquire(struct spinlock *spinlock) {
    while (!spinlock_test_acq(spinlock)) {
        __builtin_ia32_pause();
    }
}

void spinlock_release(struct spinlock *spinlock) {
    __sync_bool_compare_and_swap(&spinlock->spinlock, true, false);
}
