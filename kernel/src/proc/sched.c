#include <stdint.h>
#include <stddef.h>
#include <proc/sched.k.h>
#include <lib/printf.k.h>
#include <sys/cpu.k.h>
#include <sys/idt.k.h>
#include <sys/isr.k.h>
#include <sys/lapic.k.h>

static uint8_t sched_vector;

static void sched_isr(uint32_t int_no, struct cpu_ctx *ctx) {
    (void)int_no;

    lapic_timer_stop();




}

void sched_init(void) {
    sched_vector = idt_allocate_vector();
    printf("sched: Interrupt vector is 0x%x\n", sched_vector);

    isr_handler_table[sched_vector] = sched_isr;



}
