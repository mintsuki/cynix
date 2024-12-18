#include <stdbool.h>
#include <limine.h>
#include <mm/vmm.k.h>
#include <sys/cpu.k.h>
#include <sys/gdt.k.h>
#include <sys/idt.k.h>
#include <sys/lapic.k.h>
#include <lib/alloc.k.h>
#include <lib/vector.k.h>
#include <lib/printf.k.h>
#include <lib/panic.k.h>
#include <lib/stack.k.h>

bool x2apic_mode;

uint64_t fpu_storage_size;
void (*fpu_save)(void *);
void (*fpu_restore)(void *);

typeof(cpu_locals) cpu_locals = (typeof(cpu_locals))VECTOR_INITIALISER;

void cpu_init(struct limine_mp_info *mp_info) {
    struct cpu_local *cpu_local = (void *)mp_info->extra_argument;

    cpu_local->lapic_id = mp_info->lapic_id;

    gdt_reload();
    idt_reload();

    gdt_load_tss(&cpu_local->tss);

    vmm_switch_pagemap(&kernel_pagemap);

    cpu_local->common_int_stack = stack_new(sizeof(uint64_t), 2048);
    cpu_local->tss.rsp0 = (uintptr_t)cpu_local->common_int_stack->top;

    cpu_local->abort_stack = stack_new(sizeof(uint64_t), 512);
    cpu_local->tss.ist1 = (uintptr_t)cpu_local->abort_stack->top;

    cpu_local->idle_thread.self = &cpu_local->idle_thread;
    cpu_local->idle_thread.running_on = cpu_local->cpu_number;

    set_gs_base(&cpu_local->idle_thread);
    set_kernel_gs_base(&cpu_local->idle_thread);

    // Enable SSE/SSE2
    uint64_t cr0 = read_cr0();
    cr0 &= ~((uint64_t)1 << 2);
    cr0 |= ((uint64_t)1 << 1);
    write_cr0(cr0);

    uint64_t cr4 = read_cr4();
    cr4 |= ((uint64_t)3 << 9);
    write_cr4(cr4);

    uint32_t eax, ebx, ecx, edx;

    if (cpuid(1, 0, &eax, &ebx, &ecx, &edx) && (ecx & CPUID_XSAVE)) {
        if (cpu_local->is_bsp) {
            printf("fpu: xsave supported\n");
        }

        // Enable XSAVE and x{get, set}bv
        cr4 = read_cr4();
        cr4 |= ((uint64_t)1 << 18);
        write_cr4(cr4);

        uint64_t xcr0 = 0;
        if (cpu_local->is_bsp) {
            printf("fpu: Saving x87 state using xsave\n");
        }
        xcr0 |= ((uint64_t)1 << 0);
        if (cpu_local->is_bsp) {
            printf("fpu: Saving SSE state using xsave\n");
        }
        xcr0 |= ((uint64_t)1 << 1);

        if (ecx & CPUID_AVX) {
            if (cpu_local->is_bsp) {
                printf("fpu: Saving AVX state using xsave\n");
            }
            xcr0 |= ((uint64_t)1 << 2);
        }

        if (cpuid(7, 0, &eax, &ebx, &ecx, &edx) && (ebx & CPUID_AVX512)) {
            if (cpu_local->is_bsp) {
                printf("fpu: Saving AVX-512 state using xsave\n");
            }
            xcr0 |= ((uint64_t)1 << 5);
            xcr0 |= ((uint64_t)1 << 6);
            xcr0 |= ((uint64_t)1 << 7);
        }

        wrxcr(0, xcr0);

        if (!cpuid(0xd, 0, &eax, &ebx, &ecx, &edx)) {
            panic(NULL, "CPUID failure");
        }

        fpu_storage_size = ecx;
        fpu_save = xsave;
        fpu_restore = xrstor;
    } else {
        if (cpu_local->is_bsp) {
            printf("fpu: Using legacy fxsave\n");
        }

        fpu_storage_size = 512;
        fpu_save = fxsave;
        fpu_restore = fxrstor;
    }

    lapic_enable(0xff);

    lapic_timer_calibrate(cpu_local);

    printf("mp: CPU %u booted\n", cpu_local->cpu_number);

    cpu_local->online = true;

    if (cpu_local->is_bsp == false) {
        asm ("sti");
        for (;;) {
            asm ("pause");
        }
    }
}

struct cpu_local *cpu_current(void) {
    if (interrupt_state() != false) {
        panic(NULL, "Attempted to get current CPU struct without disabling interrupts");
    }

    uint64_t cpu_number;
    asm volatile (
        "movq %%gs:8, %0\n\t"
        : "=r"(cpu_number)
    );

    return VECTOR_ITEM(&cpu_locals, cpu_number);
}
