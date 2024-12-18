#include <stdint.h>
#include <stddef.h>
#include <lib/stack.k.h>
#include <lib/alloc.k.h>
#include <lib/panic.k.h>

struct stack *stack_new(size_t type_size, size_t nmemb) {
    struct stack *stack = alloc(sizeof(struct stack));

    stack->base = alloc(type_size * nmemb);
    stack->top = stack->base + (type_size * nmemb);
    stack->type_size = type_size;
    stack->cur_offset = type_size * nmemb;

    return stack;
}

void stack_push(struct stack *stack, const void *item) {
    if (stack->cur_offset < stack->type_size) {
        panic(NULL, "Stack push overflow");
    }

    stack->cur_offset -= stack->type_size;

    memcpy(stack->base + stack->cur_offset, item, stack->type_size);
}

void stack_pop(struct stack *stack, void *item) {
    if (stack->base + stack->cur_offset == stack->top) {
        panic(NULL, "Stack pop underflow");
    }

    memcpy(item, stack->base + stack->cur_offset, stack->type_size);

    stack->cur_offset += stack->type_size;
}
