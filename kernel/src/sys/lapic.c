#include <stdint.h>
#include <stddef.h>
#include <sys/lapic.k.h>
#include <sys/cpu.k.h>
#include <sys/pit.k.h>
#include <mm/generic.k.h>
#include <lib/printf.k.h>

static volatile uint32_t *lapic_base = NULL;

static uint32_t xapic_read(uint32_t reg) {
    if (lapic_base == NULL) {
        lapic_base = (volatile uint32_t *)((rdmsr(0x1b) & 0xfffff000) + hhdm);
    }
    return lapic_base[reg / 4];
}

static uint64_t x2apic_read(uint32_t reg) {
    return rdmsr(0x800 + (reg >> 4));
}

static void xapic_write(uint32_t reg, uint32_t val) {
    if (lapic_base == NULL) {
        lapic_base = (volatile uint32_t *)((rdmsr(0x1b) & 0xfffff000) + hhdm);
    }
    lapic_base[reg / 4] = val;
}

static void x2apic_write(uint32_t reg, uint64_t val) {
    wrmsr(0x800 + (reg >> 4), val);
}

static uint64_t lapic_read(uint32_t reg) {
    if (x2apic_mode) {
        return x2apic_read(reg);
    } else {
        return xapic_read(reg);
    }
}

static void lapic_write(uint32_t reg, uint64_t val) {
    if (x2apic_mode) {
        x2apic_write(reg, val);
    } else {
        xapic_write(reg, val);
    }
}

void lapic_timer_stop(void) {
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0);
    lapic_write(LAPIC_REG_TIMER, (1 << 16));
}

void lapic_timer_calibrate(struct cpu_local *cpu_local) {
    lapic_timer_stop();

    lapic_write(LAPIC_REG_TIMER, (1 << 16) | 0xff); // Vector 0xff, masked
    lapic_write(LAPIC_REG_TIMER_DIV, 0b1011);   // Timer divisor = 1

    uint32_t eax, ebx, ecx, edx;
    if (cpuid(0x15, 0, &eax, &ebx, &ecx, &edx) && ecx != 0) {
        cpu_local->lapic_timer_freq = ecx;
        goto out;
    }

    uint64_t samples = 16;
    for (;;) {
        pit_set_reload_value(0xfff0);

        uint64_t initial_pit_tick = pit_get_current_count();

        lapic_write(LAPIC_REG_TIMER_INITCNT, samples);

        while (lapic_read(LAPIC_REG_TIMER_CURCNT) != 0);

        uint64_t final_pit_tick = pit_get_current_count();

        uint64_t pit_ticks = initial_pit_tick - final_pit_tick;

        if (pit_ticks < 0x4000) {
            samples *= 2;
            continue;
        }

        cpu_local->lapic_timer_freq = (samples / pit_ticks) * PIT_DIVIDEND;

        break;
    }

    lapic_timer_stop();

out:
    printf("lapic: Timer frequency for CPU %u is %llu\n",
           cpu_local->cpu_number, cpu_local->lapic_timer_freq);
}

void lapic_timer_oneshot(struct cpu_local *cpu_local, uint8_t vec, uint64_t us) {
    lapic_timer_stop();

    uint64_t ticks = us * (cpu_local->lapic_timer_freq / 1000000);

    lapic_write(LAPIC_REG_TIMER, vec);
    lapic_write(LAPIC_REG_TIMER_DIV, 0b1011);
    lapic_write(LAPIC_REG_TIMER_INITCNT, ticks);
}

void lapic_enable(uint8_t spurious_vec) {
    lapic_write(LAPIC_REG_SPURIOUS, lapic_read(LAPIC_REG_SPURIOUS) | (1 << 8) | spurious_vec);
}

void lapic_eoi(void) {
    lapic_write(LAPIC_REG_EOI, 0);
}

void lapic_send_ipi(uint32_t lapic_id, uint8_t vec) {
    if (x2apic_mode) {
        x2apic_write(LAPIC_REG_ICR0, ((uint64_t)lapic_id << 32) | vec);
    } else {
        xapic_write(LAPIC_REG_ICR1, lapic_id << 24);
        xapic_write(LAPIC_REG_ICR0, vec);
    }
}
