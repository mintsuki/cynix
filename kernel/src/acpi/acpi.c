#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limine.h>
#include <lib/misc.k.h>
#include <lib/panic.k.h>
#include <lib/printf.k.h>
#include <lib/spinlock.k.h>
#include <mm/generic.k.h>
#include <mm/vmm.k.h>
#include <mm/slab.k.h>
#include <sys/port.k.h>
#include <sys/hpet.k.h>
#include <acpi/acpi.k.h>
#include <acpi/madt.k.h>
#include <uacpi/uacpi.h>
#include <uacpi/kernel_api.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request limine_rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

struct rsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t ext_checksum;
    char reserved[3];
};

struct rsdt {
    struct sdt;
    char data[];
};

static struct rsdp *rsdp = NULL;
static struct rsdt *rsdt = NULL;

static inline bool use_xsdt(void) {
    return rsdp->revision >= 2 && rsdp->xsdt_addr != 0;
}

void acpi_init(void) {
    struct limine_rsdp_response *rsdp_resp = limine_rsdp_request.response;
    if (rsdp_resp == NULL || rsdp_resp->address == 0) {
        panic(NULL, "ACPI is not supported on this machine");
    }

    rsdp = (void *)(rsdp_resp->address + hhdm);

    vmm_map_span(&kernel_pagemap,
        rsdp,
        (uintptr_t)rsdp - hhdm,
        sizeof(struct rsdp),
        VMM_PTE_PRESENT | VMM_PTE_NOEXEC | VMM_PTE_WRITABLE);

    if (use_xsdt()) {
        rsdt = (struct rsdt *)(rsdp->xsdt_addr + hhdm);
    } else {
        rsdt = (struct rsdt *)((uint64_t)rsdp->rsdt_addr + hhdm);
    }

    printf("acpi: Revision: %lu\n", rsdp->revision);
    printf("acpi: Uses XSDT? %s\n", use_xsdt() ? "true" : "false");
    printf("acpi: RSDT at %lx\n", rsdt);

    vmm_map_span(&kernel_pagemap,
        rsdt,
        (uintptr_t)rsdt - hhdm,
        sizeof(struct sdt),
        VMM_PTE_PRESENT | VMM_PTE_NOEXEC | VMM_PTE_WRITABLE);

    printf("acpi: RSDT size: %lx\n", rsdt->length);

    vmm_map_span(&kernel_pagemap,
        rsdt,
        (uintptr_t)rsdt - hhdm,
        rsdt->length,
        VMM_PTE_PRESENT | VMM_PTE_NOEXEC | VMM_PTE_WRITABLE);

    struct sdt *fadt = acpi_find_sdt("FACP", 0);
    if (fadt != NULL && fadt->length >= 116) {
        uint32_t fadt_flags = *((uint32_t *)fadt + 28);

        if ((fadt_flags & (1 << 20)) != 0) {
            panic(NULL, "HW reduced ACPI systems not supported");
        }
    }

    madt_init();

    hpet_init();

    printf("%s\n", uacpi_status_to_string(uacpi_initialize(0)));
    printf("%s\n", uacpi_status_to_string(uacpi_namespace_load()));
}

void *acpi_find_sdt(const char signature[static 4], size_t index) {
    size_t entry_count = (rsdt->length - sizeof(struct sdt)) / (use_xsdt() ? 8 : 4);

    for (size_t i = 0; i < entry_count; i++) {
        struct sdt *sdt = NULL;
        if (use_xsdt()) {
            sdt = (struct sdt *)(*((uint64_t*)rsdt->data + i) + hhdm);
        } else {
            sdt = (struct sdt *)(*((uint32_t*)rsdt->data + i) + hhdm);
        }

        vmm_map_span(&kernel_pagemap,
            sdt,
            (uintptr_t)sdt - hhdm,
            sizeof(struct sdt),
            VMM_PTE_PRESENT | VMM_PTE_NOEXEC | VMM_PTE_WRITABLE);

        if (memcmp(sdt->signature, signature, 4) != 0) {
            continue;
        }

        if (index > 0) {
            index--;
            continue;
        }

        printf("acpi: Found '%4s' at %lx, length=%lu\n", signature, sdt, sdt->length);

        vmm_map_span(&kernel_pagemap,
            sdt,
            (uintptr_t)sdt - hhdm,
            sdt->length,
            VMM_PTE_PRESENT | VMM_PTE_NOEXEC | VMM_PTE_WRITABLE);

        return sdt;
    }

    printf("acpi: Could not find '%4s'\n", signature);
    return NULL;
}

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    *out_rsdp_address = (uintptr_t)rsdp - hhdm;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle *out_handle) {
    uint64_t dev = (uint64_t)address.segment << 32 | address.bus << 16 | address.device << 8 | address.function;
    *out_handle = (void *)dev;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {
    // ...
}

uacpi_status uacpi_kernel_pci_read(uacpi_handle device, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64 *value) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write(uacpi_handle device, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64 value) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

void uacpi_kernel_free(void *mem) {
    slab_free(mem);
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return slab_alloc(size);
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    vmm_map_span(&kernel_pagemap, (void *)addr + hhdm, addr, len, VMM_PTE_PRESENT | VMM_PTE_NOEXEC | VMM_PTE_WRITABLE);
    return (void *)addr + hhdm;
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    (void)addr; (void)len;
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle) {
    *out_handle = (void *)base;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
    (void)handle;
}

uacpi_status uacpi_kernel_io_read(
    uacpi_handle handle, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value
) {
    switch (byte_width) {
        case 1: *value = inb((uintptr_t)handle + offset); break;
        case 2: *value = inw((uintptr_t)handle + offset); break;
        case 4: *value = ind((uintptr_t)handle + offset); break;
        default: panic(NULL, "uACPI I/O read of width %u\n", byte_width);
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write(
    uacpi_handle handle, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value
) {
    switch (byte_width) {
        case 1: outb((uintptr_t)handle + offset, value); break;
        case 2: outw((uintptr_t)handle + offset, value); break;
        case 4: outd((uintptr_t)handle + offset, value); break;
        default: panic(NULL, "uACPI I/O write of width %u\n", byte_width);
    }
    return UACPI_STATUS_OK;
}

void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *str) {
    (void)level;
    printf("uACPI: %s", str);
}

/*
 * Returns the number of nanosecond ticks elapsed since boot,
 * strictly monotonic.
 */
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return hpet_read_counter() * (1000000000 / hpet_frequency);
}

/*
 * Spin for N microseconds.
 */
void uacpi_kernel_stall(uacpi_u8 usec) {

}

/*
 * Sleep for N milliseconds.
 */
void uacpi_kernel_sleep(uacpi_u64 msec) {

}

/*
 * Create/free an opaque non-recursive kernel mutex object.
 */
uacpi_handle uacpi_kernel_create_mutex(void) {
    return slab_alloc(1);
}
void uacpi_kernel_free_mutex(uacpi_handle handle) {
    slab_free(handle);
}

/*
 * Create/free an opaque kernel (semaphore-like) event object.
 */
uacpi_handle uacpi_kernel_create_event(void) {
    return NULL;
}
void uacpi_kernel_free_event(uacpi_handle) {

}

/*
 * Returns a unique identifier of the currently executing thread.
 *
 * The returned thread id cannot be UACPI_THREAD_ID_NONE.
 */
uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return 0;
}

/*
 * Try to acquire the mutex with a millisecond timeout.
 *
 * The timeout value has the following meanings:
 * 0x0000 - Attempt to acquire the mutex once, in a non-blocking manner
 * 0x0001...0xFFFE - Attempt to acquire the mutex for at least 'timeout'
 *                   milliseconds
 * 0xFFFF - Infinite wait, block until the mutex is acquired
 *
 * The following are possible return values:
 * 1. UACPI_STATUS_OK - successful acquire operation
 * 2. UACPI_STATUS_TIMEOUT - timeout reached while attempting to acquire (or the
 *                           single attempt to acquire was not successful for
 *                           calls with timeout=0)
 * 3. Any other value - signifies a host internal error and is treated as such
 */
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16) {
    return UACPI_STATUS_OK;
}
void uacpi_kernel_release_mutex(uacpi_handle) {

}

/*
 * Try to wait for an event (counter > 0) with a millisecond timeout.
 * A timeout value of 0xFFFF implies infinite wait.
 *
 * The internal counter is decremented by 1 if wait was successful.
 *
 * A successful wait is indicated by returning UACPI_TRUE.
 */
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16) {
    return UACPI_FALSE;
}

/*
 * Signal the event object by incrementing its internal counter by 1.
 *
 * This function may be used in interrupt contexts.
 */
void uacpi_kernel_signal_event(uacpi_handle) {

}

/*
 * Reset the event counter to 0.
 */
void uacpi_kernel_reset_event(uacpi_handle) {

}

/*
 * Handle a firmware request.
 *
 * Currently either a Breakpoint or Fatal operators.
 */
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

/*
 * Install an interrupt handler at 'irq', 'ctx' is passed to the provided
 * handler for every invocation.
 *
 * 'out_irq_handle' is set to a kernel-implemented value that can be used to
 * refer to this handler from other API.
 */
uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
) {
    return UACPI_STATUS_OK;
}

/*
 * Uninstall an interrupt handler. 'irq_handle' is the value returned via
 * 'out_irq_handle' during installation.
 */
uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler handler, uacpi_handle irq_handle
) {
    return UACPI_STATUS_OK;
}

/*
 * Create/free a kernel spinlock object.
 *
 * Unlike other types of locks, spinlocks may be used in interrupt contexts.
 */
uacpi_handle uacpi_kernel_create_spinlock(void) {
    struct spinlock *lock = slab_alloc(sizeof(struct spinlock));
    *lock = SPINLOCK_INITIALISER;
    return lock;
}
void uacpi_kernel_free_spinlock(uacpi_handle lock) {
    slab_free(lock);
}

/*
 * Lock/unlock helpers for spinlocks.
 *
 * These are expected to disable interrupts, returning the previous state of cpu
 * flags, that can be used to possibly re-enable interrupts if they were enabled
 * before.
 *
 * Note that lock is infalliable.
 */
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle lock) {
    int old_int_state = interrupt_toggle(false);
    spinlock_acquire(lock);
    return old_int_state;
}
void uacpi_kernel_unlock_spinlock(uacpi_handle lock, uacpi_cpu_flags old_int_state) {
    spinlock_release(lock);
    interrupt_toggle(old_int_state);
}

uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type, uacpi_work_handler, uacpi_handle ctx
) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    return UACPI_STATUS_UNIMPLEMENTED;
}
