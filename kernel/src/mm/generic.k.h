#ifndef MM__GENERIC_K_H_
#define MM__GENERIC_K_H_

#include <stdint.h>
#include <stddef.h>
#include <limine.h>

#define PAGE_SIZE 4096

extern uintptr_t hhdm;
extern struct limine_memmap_entry **memmap;
extern size_t memmap_entries;

void mm_init(void);

#endif
