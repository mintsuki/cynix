#ifndef MM__PMM_K_H_
#define MM__PMM_K_H_

#include <stdint.h>
#include <stddef.h>

void pmm_init(void);
uintptr_t pmm_alloc_nozero(size_t count);
uintptr_t pmm_alloc(size_t count);
void pmm_free(uintptr_t page, size_t count);

#endif
