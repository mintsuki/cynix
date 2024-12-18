#ifndef LIB__SPINLOCK_K_H_
#define LIB__SPINLOCK_K_H_

#include <stdbool.h>

struct spinlock {
    bool spinlock;
};

#define SPINLOCK_INITIALISER ((struct spinlock){ false })

bool spinlock_test_acq(struct spinlock *spinlock);
void spinlock_acquire(struct spinlock *spinlock);
void spinlock_release(struct spinlock *spinlock);

#endif
