#include <stdint.h>
#include <sys/idt.k.h>
#include <sys/gdt.k.h>
#include <lib/panic.k.h>
#include <lib/spinlock.k.h>

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct idt_descriptor {
    uint16_t base_low16;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t base_mid16;
    uint32_t base_high32;
    uint32_t reserved;
} __attribute__((packed));

#define IDT_MAX 256

static struct idt_descriptor idt[IDT_MAX];

void idt_init(void) {
    idt_reload();
}

void idt_reload(void) {
    struct idtr idtr = {
        .limit = sizeof(idt) - 1,
        .base = (uintptr_t)idt
    };

    asm volatile (
        "lidt %0\n\t"
        :
        : "m"(idtr)
        : "memory"
    );
}

void idt_set_handler(uint8_t vector, void *isr_ptr, uint8_t ist, uint8_t flags) {
    uintptr_t isr = (uintptr_t)isr_ptr;

    idt[vector] = (struct idt_descriptor){
        .base_low16 = (uint16_t)isr,
        .selector = GDT_KERNEL_CODE_SEL,
        .ist = ist,
        .flags = flags,
        .base_mid16 = (uint16_t)(isr >> 16),
        .base_high32 = (uint32_t)(isr >> 32),
        .reserved = 0
    };
}

void idt_set_ist(uint8_t vector, uint8_t ist) {
    idt[vector].ist = ist;
}

static struct spinlock idt_lock = SPINLOCK_INITIALISER;
static uint8_t idt_free_vector = 32;

uint8_t idt_allocate_vector(void) {
    spinlock_acquire(&idt_lock);

    if (idt_free_vector == 0xf0) {
        panic(NULL, "IDT exhausted");
    }
    uint8_t ret = idt_free_vector++;

    spinlock_release(&idt_lock);

    return ret;
}
