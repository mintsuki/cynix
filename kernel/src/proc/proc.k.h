#ifndef PROC__PROC_K_H_
#define PROC__PROC_K_H_

#include <stdint.h>

struct thread {
    struct thread *self;
    uint64_t running_on;
    uint64_t errno;
    void *kernel_stack;
    void *user_stack;
    uint64_t syscall_num;
};

#endif
