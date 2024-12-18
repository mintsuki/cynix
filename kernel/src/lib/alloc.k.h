#ifndef LIB__ALLOC_K_H_
#define LIB__ALLOC_K_H_

#include <stddef.h>

void free(void *ptr);
void *alloc(size_t size);
void *realloc(void *ptr, size_t new_size);

#endif
