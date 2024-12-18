#ifndef MM__VMM_K_H_
#define MM__VMM_K_H_

#include <stdint.h>
#include <stddef.h>
#include <lib/spinlock.k.h>

#define VMM_PTE_PRESENT ((uint64_t)1 << 0)
#define VMM_PTE_WRITABLE ((uint64_t)1 << 1)
#define VMM_PTE_USER ((uint64_t)1 << 2)
#define VMM_PTE_NOEXEC ((uint64_t)1 << 63)
#define VMM_PTE_WC (((uint64_t)1 << 3) | ((uint64_t)1 << 7))
#define VMM_PTE_FLAGS_MASK ((uint64_t)0xfff | VMM_PTE_PRESENT | VMM_PTE_WRITABLE | VMM_PTE_USER | VMM_PTE_NOEXEC | VMM_PTE_WC)

struct vmm_pagemap {
    struct spinlock spinlock;
    uintptr_t top_level;
    //DYNARRAY(void *) mmap_ranges;
};

extern struct vmm_pagemap kernel_pagemap;

void vmm_init(void);
void vmm_switch_pagemap(struct vmm_pagemap *pagemap);
void vmm_map_page(struct vmm_pagemap *pagemap, void *virt, uintptr_t phys, uint64_t flags);
void vmm_map_span(struct vmm_pagemap *pagemap, void *virt, uintptr_t phys, size_t len, uint64_t flags);

#endif
