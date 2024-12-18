#include <stdint.h>
#include <stddef.h>
#include <acpi/acpi.k.h>
#include <sys/hpet.k.h>
#include <lib/printf.k.h>
#include <lib/panic.k.h>
#include <mm/generic.k.h>
#include <mm/vmm.k.h>

struct hpet_table {
    struct sdt sdt;
    uint8_t hardware_rev_id;        /* Hardware revision.                  */
    uint8_t comparator_count:5;     /* Comparator count.                   */
    uint8_t counter_size:1;         /* 1 -> 64 bits, 0 -> 32 bits.         */
    uint8_t reserved:1;             /* Reserved.                           */
    uint8_t legacy_replacement:1;   /* Support legacy ISA interrupt, bool. */
    uint16_t pci_vendor_id;
    uint8_t address_space_id;       /* 0 - system memory, 1 - system I/O.  */
    uint8_t register_bit_width;     /* Width of control registers          */
    uint8_t register_bit_offset;    /* Offset of those registers           */
    uint8_t reserved1;              /* Reserved (duh)                      */
    uint64_t address;               /* Address                             */
    uint8_t hpet_number;            /* Number of HPET.                     */
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));

struct hpet_timer {
    volatile uint64_t config_and_capabilities;
    volatile uint64_t comparator_value;
    volatile uint64_t fsb_interrupt_route;
    volatile uint64_t unused;
};

struct hpet {
    volatile uint64_t general_capabilities;
    volatile uint64_t unused0;
    volatile uint64_t general_configuration;
    volatile uint64_t unused1;
    volatile uint64_t general_int_status;
    volatile uint64_t unused2;
    volatile uint64_t unused3[2][12];
    volatile uint64_t main_counter_value;
    volatile uint64_t unused4;
    struct hpet_timer timers[];
};

static struct hpet *hpet;
uint64_t hpet_frequency;

uint64_t hpet_read_counter(void) {
    return hpet->main_counter_value;
}

void hpet_init(void) {
    uint64_t tmp;

    struct hpet_table *hpet_table = acpi_find_sdt("HPET", 0);

    if (!hpet_table) {
        panic(NULL, "HPET ACPI table not found");
    }

    hpet = (struct hpet *)(hpet_table->address + hhdm);

    vmm_map_span(&kernel_pagemap,
        hpet,
        (uintptr_t)hpet - hhdm,
        sizeof(struct hpet),
        VMM_PTE_PRESENT | VMM_PTE_NOEXEC | VMM_PTE_WRITABLE);

    tmp = hpet->general_capabilities;

    uint64_t counter_clk_period = tmp >> 32;
    hpet_frequency = 1000000000000000 / counter_clk_period;

    printf("hpet: Detected frequency of %llu Hz\n", hpet_frequency);

    hpet->main_counter_value = 0;

    printf("hpet: Enabling\n");
    tmp = hpet->general_configuration;
    tmp |= 0b01;
    hpet->general_configuration = tmp;
}
