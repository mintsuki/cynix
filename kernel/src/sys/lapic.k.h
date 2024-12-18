#ifndef SYS__LAPIC_K_H_
#define SYS__LAPIC_K_H_

#include <stdint.h>
#include <sys/cpu.k.h>

#define LAPIC_REG_ICR0 0x300
#define LAPIC_REG_ICR1 0x310
#define LAPIC_REG_SPURIOUS 0x0f0
#define LAPIC_REG_EOI 0x0b0
#define LAPIC_REG_TIMER 0x320
#define LAPIC_REG_TIMER_INITCNT 0x380
#define LAPIC_REG_TIMER_CURCNT 0x390
#define LAPIC_REG_TIMER_DIV 0x3e0

void lapic_enable(uint8_t spurious_vec);
void lapic_eoi(void);
void lapic_send_ipi(uint32_t lapic_id, uint8_t vec);
void lapic_timer_stop(void);
void lapic_timer_calibrate(struct cpu_local *cpu_local);
void lapic_timer_oneshot(struct cpu_local *cpu_local, uint8_t vec, uint64_t us);

#endif
