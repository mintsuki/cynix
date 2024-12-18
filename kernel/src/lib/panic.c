#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <lib/vector.k.h>
#include <lib/panic.k.h>
#include <lib/spinlock.k.h>
#include <lib/printf.k.h>
#include <sys/cpu.k.h>
#include <sys/lapic.k.h>
#include <sys/isr.k.h>
#include <sys/mp.k.h>

static struct spinlock panic_lock = SPINLOCK_INITIALISER;

noreturn void panic(struct cpu_ctx *ctx, const char *fmt, ...) {
    interrupt_toggle(false);

    spinlock_acquire(&panic_lock);

    if (mp_init_done) {
        VECTOR_FOR_EACH(&cpu_locals, cpu_local,
            if (!(*cpu_local)->is_bsp) {
                lapic_send_ipi((*cpu_local)->lapic_id, isr_abort_vector);
            }
        );
    }

    printf("\n*** KERNEL PANIC ***\n\n");
    printf("Reason for panic: ");

    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);
    printf("\n\n");

    if (ctx == NULL) {
        goto halt;
    }

    uint64_t cr2 = read_cr2();
    uint64_t cr3 = read_cr3();
    printf("CPU context at panic:\n"
           "  RAX=%016lx  RBX=%016lx\n"
           "  RCX=%016lx  RDX=%016lx\n"
           "  RSI=%016lx  RDI=%016lx\n"
           "  RBP=%016lx  RSP=%016lx\n"
           "  R08=%016lx  R09=%016lx\n"
           "  R10=%016lx  R11=%016lx\n"
           "  R12=%016lx  R13=%016lx\n"
           "  R14=%016lx  R15=%016lx\n"
           "  RIP=%016lx  RFLAGS=%08lx\n"
           "  CS=%04lx DS=%04lx ES=%04lx SS=%04lx\n"
           "  CR2=%016lx  CR3=%016lx\n"
           "  ERR=%016lx\n",
           ctx->rax, ctx->rbx, ctx->rcx, ctx->rdx,
           ctx->rsi, ctx->rdi, ctx->rbp, ctx->rsp,
           ctx->r8, ctx->r9, ctx->r10, ctx->r11,
           ctx->r12, ctx->r13, ctx->r14, ctx->r15,
           ctx->rip, ctx->rflags,
           ctx->cs, ctx->ds, ctx->es, ctx->ss,
           cr2, cr3, ctx->err);

halt:
    printf("System halted.");

    for (;;) {
        halt();
    }
}
