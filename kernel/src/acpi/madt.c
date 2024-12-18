#include <stddef.h>
#include <stdint.h>
#include <lib/printf.k.h>
#include <lib/panic.k.h>
#include <lib/misc.k.h>
#include <lib/vector.k.h>
#include <acpi/acpi.k.h>
#include <acpi/madt.k.h>
#include <sys/cpu.k.h>

typeof(madt_lapics) madt_lapics = (typeof(madt_lapics))VECTOR_INITIALISER;
typeof(madt_x2apics) madt_x2apics = (typeof(madt_x2apics))VECTOR_INITIALISER;
typeof(madt_io_apics) madt_io_apics = (typeof(madt_io_apics))VECTOR_INITIALISER;
typeof(madt_isos) madt_isos = (typeof(madt_isos))VECTOR_INITIALISER;
typeof(madt_nmis) madt_nmis = (typeof(madt_nmis))VECTOR_INITIALISER;

struct madt {
    struct sdt;
    uint32_t local_controller_addr;
    uint32_t flags;
    char entries_data[];
};

void madt_init(void) {
    struct madt *madt = acpi_find_sdt("APIC", 0);
    if (madt == NULL) {
        panic(NULL, "System does not have an MADT");
    }

    size_t offset = 0;
    for (;;) {
        if (offset + sizeof(struct madt) >= madt->length) {
            break;
        }

        struct madt_header *header = (struct madt_header *)(madt->entries_data + offset);
        switch (header->id) {
            case 0:
                VECTOR_PUSH_BACK(&madt_lapics, (struct madt_lapic *)header);
                break;
            case 9:
                if (x2apic_mode) {
                    VECTOR_PUSH_BACK(&madt_x2apics, (struct madt_x2apic *)header);
                }
                break;
            case 1:
                printf("madt: Found IO APIC #%lu\n", madt_io_apics.length);
                VECTOR_PUSH_BACK(&madt_io_apics, (struct madt_io_apic *)header);
                break;
            case 2:
                printf("madt: Found ISO #%lu\n", madt_isos.length);
                VECTOR_PUSH_BACK(&madt_isos, (struct madt_iso *)header);
                break;
            case 4:
                VECTOR_PUSH_BACK(&madt_nmis, (struct madt_nmi *)header);
                break;
        }

        offset += header->length;
    }
}
