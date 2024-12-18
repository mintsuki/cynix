#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <lib/printf.k.h>
#include <lib/panic.k.h>
#include <devices/term/term.k.h>
#include <mm/generic.k.h>
#include <mm/pmm.k.h>
#include <mm/vmm.k.h>
#include <mm/slab.k.h>
#include <sys/cpu.k.h>
#include <sys/gdt.k.h>
#include <sys/idt.k.h>
#include <sys/isr.k.h>
#include <sys/mp.k.h>
#include <acpi/acpi.k.h>
#include <proc/sched.k.h>

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        for (;;);
    }

    pmm_init();
    slab_init();

    term_init(false);

    gdt_init();
    idt_init();
    isr_init();

    vmm_init();

    if (limine_mp_request.response == NULL) {
        panic(NULL, "MP bootloader response missing");
    }
    x2apic_mode = limine_mp_request.response->flags & LIMINE_MP_X2APIC;

    acpi_init();
    mp_init();

    sched_init();

    panic(NULL, "Nothing to do");
}
