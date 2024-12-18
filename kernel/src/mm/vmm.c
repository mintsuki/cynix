#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <elf.h>
#include <limine.h>
#include <mm/vmm.k.h>
#include <mm/pmm.k.h>
#include <mm/generic.k.h>
#include <lib/misc.k.h>
#include <lib/printf.k.h>
#include <lib/panic.k.h>
#include <lib/spinlock.k.h>
#include <sys/cpu.k.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request limine_executable_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_paging_mode_request limine_paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 1,
    .mode = LIMINE_PAGING_MODE_X86_64_5LVL,
    .max_mode = LIMINE_PAGING_MODE_X86_64_5LVL,
    .min_mode = LIMINE_PAGING_MODE_X86_64_4LVL
};

struct vmm_pagemap kernel_pagemap = {
    .spinlock = SPINLOCK_INITIALISER
};

static bool vmm_la57 = false;

void vmm_switch_pagemap(struct vmm_pagemap *pagemap) {
    asm volatile (
        "mov %0, %%cr3\n\t"
        :
        : "r"(pagemap->top_level)
        : "memory"
    );
}

static uintptr_t get_next_level(uintptr_t current_level, uint64_t index, bool allocate) {
    uintptr_t ret;

    uint64_t *entry = (void *)current_level + hhdm + index * 8;

    if ((*entry & 0x01) != 0) {
        ret = *entry & ~VMM_PTE_FLAGS_MASK;
    } else {
        if (allocate == false) {
            return 0;
        }

        ret = pmm_alloc(1);
        *entry = ret | 0b111;
    }

    return ret;
}

void vmm_map_page(struct vmm_pagemap *pagemap, void *virt, uintptr_t phys, uint64_t flags) {
    bool old_int_state = interrupt_toggle(false);

    spinlock_acquire(&pagemap->spinlock);

    size_t pml5_index = ((uint64_t)virt & ((uint64_t)0x1ff << 48)) >> 48;
    size_t pml4_index = ((uint64_t)virt & ((uint64_t)0x1ff << 39)) >> 39;
    size_t pml3_index = ((uint64_t)virt & ((uint64_t)0x1ff << 30)) >> 30;
    size_t pml2_index = ((uint64_t)virt & ((uint64_t)0x1ff << 21)) >> 21;
    size_t pml1_index = ((uint64_t)virt & ((uint64_t)0x1ff << 12)) >> 12;

    uintptr_t pml5 = pagemap->top_level;
    uintptr_t pml4 = vmm_la57 ? get_next_level(pml5, pml5_index, true) : pagemap->top_level;
    uintptr_t pml3 = get_next_level(pml4, pml4_index, true);
    uintptr_t pml2 = get_next_level(pml3, pml3_index, true);
    uintptr_t pml1 = get_next_level(pml2, pml2_index, true);

    uint64_t *entry = (void *)pml1 + hhdm + pml1_index * 8;

    *entry = phys | flags;

    spinlock_release(&pagemap->spinlock);

    interrupt_toggle(old_int_state);
}

void vmm_map_span(struct vmm_pagemap *pagemap, void *virt, uintptr_t phys, size_t len, uint64_t flags) {
    uintptr_t aligned_phys_top = ALIGN_UP(phys + len, PAGE_SIZE);
    uintptr_t aligned_phys_base = ALIGN_DOWN(phys, PAGE_SIZE);

    void *aligned_virt_base = (void *)ALIGN_DOWN((uintptr_t)virt, PAGE_SIZE);

    size_t aligned_length = aligned_phys_top - aligned_phys_base;

    for (size_t i = 0; i < aligned_length; i += PAGE_SIZE) {
        vmm_map_page(pagemap, aligned_virt_base + i, aligned_phys_base + i, flags);
    }
}

static void map_kernel_span(void *virt, uintptr_t phys, ptrdiff_t len, uint64_t flags) {
    size_t aligned_len = ALIGN_UP(len, PAGE_SIZE);

    printf("vmm: Kernel: Mapping %p to %p, length 0x%llx\n", phys, virt, aligned_len);

    for (size_t i = 0; i < aligned_len; i += PAGE_SIZE) {
        vmm_map_page(&kernel_pagemap, virt + i, phys + i, flags);
    }
}

void vmm_init(void) {
    if (limine_paging_mode_request.response != NULL) {
        if (limine_paging_mode_request.response->mode == LIMINE_PAGING_MODE_X86_64_5LVL) {
            printf("vmm: Using 5 level paging\n");
            vmm_la57 = true;
        }
    }

    kernel_pagemap.top_level = pmm_alloc(1);

    // Since the higher half has to be shared amongst all address spaces,
    // we need to initialise every single higher half PML3 so they can be
    // shared.
    for (size_t i = 256; i < 512; i++) {
        // get_next_level will allocate the PML3s for us.
        if (get_next_level(kernel_pagemap.top_level, i, true) == 0) {
            panic(NULL, "vmm: init failure");
        }
    }

    // Map kernel
    struct limine_executable_address_response *exec_addr_resp = limine_executable_address_request.response;
    if (exec_addr_resp == NULL) {
        panic(NULL, "Executable address bootloader response missing");
    }
    printf("vmm: Kernel physical base: %p\n", exec_addr_resp->physical_base);
    printf("vmm: Kernel virtual base: %p\n", exec_addr_resp->virtual_base);
    uint64_t physical_base = exec_addr_resp->physical_base;
    uint64_t virtual_base = exec_addr_resp->virtual_base;

    void *kernel_file = limine_executable_file_request.response->executable_file->address;

    Elf64_Ehdr *kernel_ehdr = kernel_file;
    Elf64_Phdr *kernel_phdr = kernel_file + kernel_ehdr->e_phoff;

    for (size_t i = 0; i < kernel_ehdr->e_phnum; i++,
         kernel_phdr = (void *)kernel_phdr + kernel_ehdr->e_phentsize
    ) {
        if (kernel_phdr->p_type != PT_LOAD) {
            continue;
        }

        uintptr_t phys = (kernel_phdr->p_vaddr - virtual_base) + physical_base;

        uint64_t flags = VMM_PTE_PRESENT;
        if ((kernel_phdr->p_flags & PF_X) == 0) {
            flags |= VMM_PTE_NOEXEC;
        }
        if ((kernel_phdr->p_flags & PF_W) != 0) {
            flags |= VMM_PTE_WRITABLE;
        }

        map_kernel_span((void *)kernel_phdr->p_vaddr, phys, kernel_phdr->p_memsz, flags);
    }

    for (size_t i = 0; i < memmap_entries; i++) {
        if (memmap[i]->type != LIMINE_MEMMAP_USABLE
         && memmap[i]->type != LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
         && memmap[i]->type != LIMINE_MEMMAP_EXECUTABLE_AND_MODULES
         && memmap[i]->type != LIMINE_MEMMAP_FRAMEBUFFER) {
            continue;
        }

        uintptr_t base = ALIGN_DOWN(memmap[i]->base, PAGE_SIZE);
        uintptr_t top = ALIGN_UP(memmap[i]->base + memmap[i]->length, PAGE_SIZE);

        for (uintptr_t j = base; j < top; j += PAGE_SIZE) {
            vmm_map_page(&kernel_pagemap, (void *)j + hhdm, j,
                VMM_PTE_PRESENT | VMM_PTE_NOEXEC | VMM_PTE_WRITABLE |
                (memmap[i]->type == LIMINE_MEMMAP_FRAMEBUFFER ? VMM_PTE_WC : 0));
        }
    }

    vmm_switch_pagemap(&kernel_pagemap);
}
