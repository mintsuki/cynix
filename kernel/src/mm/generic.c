#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <mm/generic.k.h>
#include <lib/panic.k.h>

static bool mm_init_done = false;

uintptr_t hhdm;
struct limine_memmap_entry **memmap;
size_t memmap_entries;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request limine_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

void mm_init(void) {
    if (mm_init_done == true) {
        return;
    }

    if (limine_hhdm_request.response == NULL) {
        panic(NULL, "HHDM bootloader response missing");
    }

    hhdm = limine_hhdm_request.response->offset;

    if (limine_memmap_request.response == NULL) {
        panic(NULL, "Memory map bootloader response missing");
    }

    memmap = limine_memmap_request.response->entries;
    memmap_entries = limine_memmap_request.response->entry_count;

    mm_init_done = true;
}
