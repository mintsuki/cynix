#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limine.h>
#include <mm/slab.k.h>
#include <lib/spinlock.k.h>
#include <lib/misc.k.h>
#include <lib/printf.k.h>
#include <mm/generic.k.h>
#include <mm/pmm.k.h>
#include <mm/vmm.k.h>

struct slab {
    struct spinlock lock;
    void **first_free;
    size_t ent_size;
};

struct slab_header {
    struct slab *slab;
};

static struct slab slabs[12];

static inline uint64_t bsr(const uint64_t x) {
  uint64_t y;
  asm ( "\tbsr %1, %0\n"
      : "=r"(y)
      : "r" (x)
  );
  return y;
}

static inline struct slab *slab_for(size_t size) {
    switch (bsr(size)) {
        case 0:
        case 1:
        case 2: return &slabs[0];
        case 3: return &slabs[1];
        case 4: return &slabs[2];
        case 5: return &slabs[3];
        case 6: return &slabs[4];
        case 7: return &slabs[5];
        case 8: return &slabs[6];
        case 9: return &slabs[7];
        case 10: return &slabs[8];
        case 11: return &slabs[9];
        case 12: return &slabs[10];
        case 13: return &slabs[11];
        default: return NULL;
    }
}

static void create_slab(struct slab *slab, size_t ent_size) {
    slab->lock = SPINLOCK_INITIALISER;
    slab->first_free = (void *)pmm_alloc_nozero(64) + hhdm;

    slab->ent_size = ent_size;

    size_t header_offset = ALIGN_UP(sizeof(struct slab_header), ent_size);
    size_t available_size = PAGE_SIZE * 64 - header_offset;

    struct slab_header *slab_ptr = (struct slab_header *)slab->first_free;
    slab_ptr->slab = slab;
    slab->first_free = (void **)((void *)slab->first_free + header_offset);

    void **arr = (void **)slab->first_free;
    size_t max = (available_size >> bsr(ent_size)) - 1;
    size_t fact = ent_size >> 3;

    for (size_t i = 0; i < max; i++) {
        arr[i * fact] = &arr[(i + 1) * fact];
    }
    arr[max * fact] = NULL;
}

static void *alloc_from_slab(struct slab *slab) {
    spinlock_acquire(&slab->lock);

    if (slab->first_free == NULL) {
        create_slab(slab, slab->ent_size);
    }

    void **old_free = slab->first_free;
    slab->first_free = *old_free;

    spinlock_release(&slab->lock);
    return old_free;
}

static void free_in_slab(struct slab *slab, void *addr) {
    spinlock_acquire(&slab->lock);

    if (addr == NULL) {
        goto cleanup;
    }

    void **new_head = addr;
    *new_head = slab->first_free;
    slab->first_free = new_head;

cleanup:
    spinlock_release(&slab->lock);
}

void slab_init(void) {
    create_slab(&slabs[0], 8);
    create_slab(&slabs[1], 16);
    create_slab(&slabs[2], 32);
    create_slab(&slabs[3], 64);
    create_slab(&slabs[4], 128);
    create_slab(&slabs[5], 256);
    create_slab(&slabs[6], 512);
    create_slab(&slabs[7], 1024);
    create_slab(&slabs[8], 2048);
    create_slab(&slabs[9], 4096);
    create_slab(&slabs[10], 8192);
    create_slab(&slabs[11], 16384);
}

void *slab_alloc(size_t size) {
    struct slab *slab = slab_for(size);
    if (slab != NULL) {
        return alloc_from_slab(slab);
    }

    return NULL;
}

void *slab_realloc(void *addr, size_t new_size) {
    if (addr == NULL) {
        return slab_alloc(new_size);
    }

    struct slab_header *slab_header = (struct slab_header *)((uintptr_t)addr & ~(uintptr_t)0x3ffff);
    struct slab *slab = slab_header->slab;

    if (new_size > slab->ent_size) {
        void *new_addr = slab_alloc(new_size);
        if (new_addr == NULL) {
            return NULL;
        }

        memcpy(new_addr, addr, slab->ent_size);
        free_in_slab(slab, addr);
        return new_addr;
    }

    return addr;
}

void slab_free(void *addr) {
    if (addr == NULL) {
        return;
    }

    struct slab_header *slab_header = (struct slab_header *)((uintptr_t)addr & ~(uintptr_t)0x3ffff);
    free_in_slab(slab_header->slab, addr);
}
