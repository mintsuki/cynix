#include <stdint.h>
#include <sys/gdt.k.h>
#include <lib/misc.k.h>
#include <lib/spinlock.k.h>

static struct spinlock gdt_lock = SPINLOCK_INITIALISER;

struct gdtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct gdt_descriptor {
    uint16_t limit;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high8;
} __attribute__((packed));

#define GDT_MAX 7

static struct gdt_descriptor gdt[GDT_MAX];

void gdt_init(void) {
    gdt[0] = (struct gdt_descriptor){0};

    gdt[1] = (struct gdt_descriptor){
        .limit = 0,
        .base_low16 = 0,
        .base_mid8 = 0,
        .access = 0b10011011,
        .granularity = 0b00100000,
        .base_high8 = 0
    };

    gdt[2] = (struct gdt_descriptor){
        .limit = 0,
        .base_low16 = 0,
        .base_mid8 = 0,
        .access = 0b10010011,
        .granularity = 0,
        .base_high8 = 0
    };

    gdt[3] = (struct gdt_descriptor){
        .limit = 0,
        .base_low16 = 0,
        .base_mid8 = 0,
        .access = 0b11111011,
        .granularity = 0b00100000,
        .base_high8 = 0
    };

    gdt[4] = (struct gdt_descriptor){
        .limit = 0,
        .base_low16 = 0,
        .base_mid8 = 0,
        .access = 0b11110011,
        .granularity = 0,
        .base_high8 = 0
    };

    gdt_reload();
}

void gdt_reload(void) {
    struct gdtr gdtr = {
        .limit = sizeof(gdt) - 1,
        .base = (uintptr_t)gdt
    };

    asm volatile (
        "lgdt %0\n\t"
        "push %%rax\n\t"
        "push $" STR(GDT_KERNEL_CODE_SEL) "\n\t"
        "lea 0x03(%%rip), %%rax\n\t"
        "push %%rax\n\t"
        "lretq\n\t"
        "mov $" STR(GDT_KERNEL_DATA_SEL) ", %%eax\n\t"
        "mov %%eax, %%ds\n\t"
        "mov %%eax, %%es\n\t"
        "mov %%eax, %%ss\n\t"
        "mov $" STR(GDT_USER_DATA_SEL) ", %%eax\n\t"
        "mov %%eax, %%fs\n\t"
        "mov %%eax, %%gs\n\t"
        "pop %%rax\n\t"
        :
        : "m"(gdtr)
        : "memory"
    );
}

void gdt_load_tss(struct cpu_tss *tss) {
    spinlock_acquire(&gdt_lock);

    uintptr_t tss_int = (uintptr_t)tss;

    gdt[5] = (struct gdt_descriptor){
        .limit = sizeof(struct cpu_tss) - 1,
        .base_low16 = tss_int,
        .base_mid8 = tss_int >> 16,
        .access = 0b10001001,
        .granularity = 0,
        .base_high8 = tss_int >> 24,
    };

    gdt[6] = (struct gdt_descriptor){
        .limit = tss_int >> 32,
        .base_low16 = tss_int >> 48
    };

    asm volatile (
        "ltr %0\n\t"
        :
        : "r" ((uint16_t)GDT_TSS_SEL)
        : "memory"
    );

    spinlock_release(&gdt_lock);
}
