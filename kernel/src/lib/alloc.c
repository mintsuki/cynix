#include <stdint.h>
#include <stddef.h>
#include <lib/alloc.k.h>
#include <mm/generic.k.h>
#include <mm/pmm.k.h>
#include <lib/misc.k.h>

struct alloc_metadata {
    size_t pages;
    size_t size;
};

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    struct alloc_metadata *metadata = ptr - PAGE_SIZE;

    pmm_free((uintptr_t)metadata - hhdm, metadata->pages + 1);
}

void *alloc(size_t size) {
    size_t page_count = DIV_ROUNDUP(size, PAGE_SIZE);

    uintptr_t phys_page = pmm_alloc(page_count + 1);

    struct alloc_metadata *metadata = (void *)(phys_page + hhdm);

    metadata->pages = page_count;
    metadata->size = size;

    return (void *)(phys_page + hhdm + PAGE_SIZE);
}

void *realloc(void *ptr, size_t new_size) {
    if (ptr == NULL) {
        return alloc(new_size);
    }

    struct alloc_metadata *metadata = ptr - PAGE_SIZE;

    if (metadata->pages == DIV_ROUNDUP(new_size, PAGE_SIZE)) {
        metadata->size = new_size;
        return ptr;
    }

    void *new_ptr = alloc(new_size);

    memcpy(new_ptr, ptr, MIN(new_size, metadata->size));

    free(ptr);

    return new_ptr;
}
