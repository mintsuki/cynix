#include <stdint.h>
#include <lib/panic.k.h>
#include <lib/misc.k.h>
#include <sys/isr.k.h>
#include <sys/gdt.k.h>
#include <sys/idt.k.h>
#include <sys/cpu.k.h>
#include <signal.h>

void *isr_handler_table[256];

uint8_t isr_abort_vector;

static const char *isr_exception_names[] = {
    "Division exception",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "???",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "???",
    "x87 exception",
    "Alignment check",
    "Machine check",
    "SIMD exception",
    "Virtualisation"
};

static void isr_abort_handler(uint32_t int_number, struct cpu_ctx *ctx) {
    (void)int_number; (void)ctx;
    for (;;) {
        asm ("hlt");
    }
}

static void isr_generic_exception_handler(uint32_t int_number, struct cpu_ctx *ctx) {
    const char *exception_name = int_number < SIZEOF_ARRAY(isr_exception_names)
        ? isr_exception_names[int_number] : "???";

    if (ctx->cs == GDT_USER_CODE_SEL) {
        //uint8_t signal = 0;

        switch (int_number) {
            //case 0x0d: case 0x0e:
            //    signal = SIGSEGV;
            //    break;
            default:
                panic(ctx, exception_name);
                break;
        }

        //send_signal(sched_current_thread(), signal);
        //syscall_exit(NULL, 128 + signal);
    } else {
        panic(ctx, exception_name);
    }
}

void isr_init(void) {
    for (uint8_t i = 0; i < 32; i++) {
        switch (i) {
/*
            case 0x0e: // page fault
                idt_set_handler(i, isr_thunks[i], 3, 0x8e);
                isr_handler_table[i] = isr_pf_handler;
                break;
*/
            default:
                idt_set_handler(i, isr_thunks[i], 0, 0x8e);
                isr_handler_table[i] = isr_generic_exception_handler;
                break;
        }
    }

    for (uint8_t i = 32; ; i++) {
        idt_set_handler(i, isr_thunks[i], 0, 0x8e);
        isr_handler_table[i] = isr_generic_exception_handler;

        // avoid overflow
        if (i == 255) {
            break;
        }
    }

    isr_abort_vector = idt_allocate_vector();
    idt_set_handler(isr_abort_vector, isr_thunks[isr_abort_vector], CPU_ABORT_IST, 0x8e);
    isr_handler_table[isr_abort_vector] = isr_abort_handler;
}
