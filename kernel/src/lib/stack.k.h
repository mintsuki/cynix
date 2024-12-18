#ifndef LIB__STACK_K_H_
#define LIB__STACK_K_H_

#include <stdint.h>
#include <stddef.h>

struct stack {
    void *base;
    void *top;
    size_t type_size;
    size_t cur_offset;
};

struct stack *stack_new(size_t type_size, size_t nmemb);
void stack_push(struct stack *stack, const void *item);
void stack_pop(struct stack *stack, void *item);

#endif
