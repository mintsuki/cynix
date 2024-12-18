#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <mm/pmm.k.h>
#include <mm/generic.k.h>
#include <lib/misc.k.h>
#include <lib/spinlock.k.h>
#include <lib/printf.k.h>
#include <lib/panic.k.h>
#include <sys/cpu.k.h>

static struct spinlock pmm_lock = SPINLOCK_INITIALISER;
static void *pmm_bitmap;

static size_t pmm_avl_page_count;

static bool pmm_init_done = false;

static const char *memmap_ent_type_to_str(uint64_t type) {
    switch (type) {
        case LIMINE_MEMMAP_USABLE:
            return "Usable";
        case LIMINE_MEMMAP_RESERVED:
            return "Reserved";
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            return "ACPI Reclaimable";
        case LIMINE_MEMMAP_ACPI_NVS:
            return "ACPI NVS";
        case LIMINE_MEMMAP_BAD_MEMORY:
            return "Bad Memory";
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            return "Bootloader Reclaimable";
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
            return "Executable and Modules";
        case LIMINE_MEMMAP_FRAMEBUFFER:
            return "Framebuffer";
        default:
            return "???";
    }
}

void pmm_init(void) {
    if (pmm_init_done == true) {
        return;
    }

    mm_init();

    uintptr_t top_address = 0;

    printf("pmm: Bootloader reported memory map:\n");

    for (size_t i = 0; i < memmap_entries; i++) {
        uint64_t entry_top = ALIGN_UP(memmap[i]->base + memmap[i]->length, PAGE_SIZE);

        printf("pmm: %3d: %016llx -> %016llx  [%s]\n",
            i, memmap[i]->base, entry_top,
            memmap_ent_type_to_str(memmap[i]->type)
        );

        if (memmap[i]->type != LIMINE_MEMMAP_USABLE
         && memmap[i]->type != LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
         && memmap[i]->type != LIMINE_MEMMAP_EXECUTABLE_AND_MODULES) {
            continue;
        }

        if (entry_top > top_address) {
            top_address = entry_top;
        }
    }

    pmm_avl_page_count = DIV_ROUNDUP(top_address, PAGE_SIZE);
    size_t bitmap_size = ALIGN_UP(pmm_avl_page_count / 8, PAGE_SIZE);

    printf("pmm: Bitmap size: %llu\n", bitmap_size);

    for (size_t i = 0; i < memmap_entries; i++) {
        if (memmap[i]->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }
        if (memmap[i]->length > bitmap_size) {
            pmm_bitmap = (void *)(memmap[i]->base + hhdm);

            memset(pmm_bitmap, 0xff, bitmap_size);

            break;
        }
    }

    uint64_t pmm_bitmap_phys = (uint64_t)pmm_bitmap - hhdm;
    printf("pmm: Bitmap at %p\n", pmm_bitmap_phys);

    for (size_t i = 0; i < memmap_entries; i++) {
        if (memmap[i]->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }
        for (size_t j = 0; j < memmap[i]->length; j += PAGE_SIZE) {
            uint64_t base = memmap[i]->base + j;
            uint64_t top = base + PAGE_SIZE;
            if ((base >= pmm_bitmap_phys && base < pmm_bitmap_phys + bitmap_size)
             || (top > pmm_bitmap_phys && top <= pmm_bitmap_phys + bitmap_size)) {
                continue;
            }
            bit_reset(pmm_bitmap, base / PAGE_SIZE);
        }
    }

    pmm_init_done = true;
}

static size_t pmm_last_used_index = 0;

static uintptr_t pmm_inner_alloc(size_t count, size_t limit) {
    size_t p = 0;

    while (pmm_last_used_index < limit) {
        pmm_last_used_index++;

        if (!bit_test(pmm_bitmap, pmm_last_used_index - 1)) {
            p++;

            if (p == count) {
                size_t page_index = pmm_last_used_index - count;
                for (size_t i = page_index; i < pmm_last_used_index; i++) {
                    bit_set(pmm_bitmap, i);
                }
                return (uintptr_t)(page_index * PAGE_SIZE);
            }
        } else {
            p = 0;
        }
    }

    return (uintptr_t)-1LL;
}

uintptr_t pmm_alloc_nozero(size_t count) {
    bool old_int_state = interrupt_toggle(false);

    spinlock_acquire(&pmm_lock);

    size_t last = pmm_last_used_index;
    uintptr_t ret = pmm_inner_alloc(count, pmm_avl_page_count);

    if (ret == (uintptr_t)-1LL) {
        pmm_last_used_index = 0;

        ret = pmm_inner_alloc(count, last);
        if (ret == (uintptr_t)-1LL) {
            panic(NULL, "Out of memory");
        }
    }

    spinlock_release(&pmm_lock);

    interrupt_toggle(old_int_state);

    return ret;
}

uintptr_t pmm_alloc(size_t count) {
    uintptr_t ret = pmm_alloc_nozero(count);

    uint64_t *ptr = (void *)(ret + hhdm);
    for (size_t i = 0; i < (count * PAGE_SIZE) / sizeof(uint64_t); i++) {
        ptr[i] = 0;
    }

    return ret;
}

void pmm_free(uintptr_t page, size_t count) {
    uint64_t *ptr = (void *)(page + hhdm);
    for (size_t i = 0; i < (count * PAGE_SIZE) / sizeof(uint64_t); i++) {
        ptr[i] = 0xaaaaaaaaaaaaaaaa;
    }

    page /= PAGE_SIZE;

    bool old_int_state = interrupt_toggle(false);

    spinlock_acquire(&pmm_lock);

    for (size_t i = page; i < page + count; i++) {
        bit_reset(pmm_bitmap, i);
    }

    spinlock_release(&pmm_lock);

    interrupt_toggle(old_int_state);
}
