#ifndef SYS__CPU_K_H_
#define SYS__CPU_K_H_

#include <stdint.h>
#include <stdbool.h>
#include <limine.h>
#include <lib/vector.k.h>
#include <lib/stack.k.h>
#include <proc/proc.k.h>

extern bool x2apic_mode;
extern uint64_t fpu_storage_size;
extern void (*fpu_save)(void *);
extern void (*fpu_restore)(void *);

#define CPU_ABORT_IST 1

struct cpu_ctx {
    uint64_t ds;
    uint64_t es;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t err;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

struct cpu_tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint32_t iopb;
} __attribute__((packed));

struct cpu_local {
    uint64_t cpu_number;
    uint32_t lapic_id;
    bool is_bsp;
    volatile bool online;
    volatile bool is_idle;
    uint64_t lapic_timer_freq;
    struct cpu_tss tss;
    struct stack *common_int_stack;
    struct stack *abort_stack;
    struct thread idle_thread;
};

extern VECTOR_TYPE(struct cpu_local *) cpu_locals;

static inline uint64_t read_cr0(void) {
    uint64_t ret;
    asm volatile ("mov %%cr0, %0" : "=r"(ret) :: "memory");
    return ret;
}

static inline uint64_t read_cr2(void) {
    uint64_t ret;
    asm volatile ("mov %%cr2, %0" : "=r"(ret) :: "memory");
    return ret;
}

static inline uint64_t read_cr3(void) {
    uint64_t ret;
    asm volatile ("mov %%cr3, %0" : "=r"(ret) :: "memory");
    return ret;
}

static inline uint64_t read_cr4(void) {
    uint64_t ret;
    asm volatile ("mov %%cr4, %0" : "=r"(ret) :: "memory");
    return ret;
}

static inline void write_cr0(uint64_t value) {
    asm volatile ("mov %0, %%cr0" :: "r"(value) : "memory");
}

static inline void write_cr2(uint64_t value) {
    asm volatile ("mov %0, %%cr2" :: "r"(value) : "memory");
}

static inline void write_cr3(uint64_t value) {
    asm volatile ("mov %0, %%cr3" :: "r"(value) : "memory");
}

static inline void write_cr4(uint64_t value) {
    asm volatile ("mov %0, %%cr4" :: "r"(value) : "memory");
}

static inline void wrxcr(uint32_t reg, uint64_t value) {
    uint32_t a = value;
    uint32_t d = value >> 32;
    asm volatile ("xsetbv" :: "a"(a), "d"(d), "c"(reg) : "memory");
}

static inline void xsave(void *ctx) {
    asm volatile (
        "xsave (%0)"
        :
        : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
        : "memory");
}

static inline void xrstor(void *ctx) {
    asm volatile (
        "xrstor (%0)"
        :
        : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
        : "memory");
}

static inline void fxsave(void *ctx) {
    asm volatile (
        "fxsave (%0)"
        :
        : "r"(ctx)
        : "memory");
}

static inline void fxrstor(void *ctx) {
    asm volatile (
        "fxrstor (%0)"
        :
        : "r"(ctx)
        : "memory");
}

static inline uint64_t rdtsc(void) {
    uint32_t edx, eax;
    asm volatile ("rdtsc" : "=d"(edx), "=a"(eax));
    return ((uint64_t)edx << 32) | edx;
}

static inline uint64_t rdrand(void) {
    uint64_t result;
    asm volatile ("rdrand %0" : "=r"(result));
    return result;
}

static inline uint64_t rdseed(void) {
    uint64_t result;
    asm volatile ("rdseed %0" : "=r"(result));
    return result;
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t edx = 0, eax = 0;
    asm volatile (
        "rdmsr\n\t"
        : "=a" (eax), "=d" (edx)
        : "c" (msr)
        : "memory"
    );
    return ((uint64_t)edx << 32) | eax;
}

static inline uint64_t wrmsr(uint32_t msr, uint64_t val) {
    uint32_t eax = (uint32_t)val;
    uint32_t edx = (uint32_t)(val >> 32);
    asm volatile (
        "wrmsr\n\t"
        :
        : "a" (eax), "d" (edx), "c" (msr)
        : "memory"
    );
    return ((uint64_t)edx << 32) | eax;
}

static inline void set_kernel_gs_base(void *addr) {
    wrmsr(0xc0000102, (uint64_t)addr);
}

static inline void set_gs_base(void *addr) {
    wrmsr(0xc0000101, (uint64_t)addr);
}

static inline void set_fs_base(void *addr) {
    wrmsr(0xc0000100, (uint64_t)addr);
}

static inline void *get_kernel_gs_base(void) {
    return (void *)rdmsr(0xc0000102);
}

static inline void *get_gs_base(void) {
    return (void *)rdmsr(0xc0000101);
}

static inline void *get_fs_base(void) {
    return (void *)rdmsr(0xc0000100);
}

#define CPUID_XSAVE ((uint32_t)1 << 26)
#define CPUID_AVX ((uint32_t)1 << 28)
#define CPUID_AVX512 ((uint32_t)1 << 16)
#define CPUID_SEP ((uint32_t)1 << 11)

static inline bool cpuid(uint32_t leaf, uint32_t subleaf,
                         uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    uint32_t cpuid_max;
    asm volatile (
        "cpuid"
        : "=a"(cpuid_max)
        : "a"(leaf & 0x80000000)
        : "rbx", "rcx", "rdx"
    );
    if (leaf > cpuid_max) {
        return false;
    }
    asm volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
    return true;
}

static inline bool interrupt_state(void) {
    uint64_t flags;
    asm volatile ("pushfq; pop %0" : "=rm"(flags) :: "memory");
    return flags & (1 << 9);
}

static inline void enable_interrupts(void) {
    asm ("sti");
}

static inline void disable_interrupts(void) {
    asm ("cli");
}

static inline bool interrupt_toggle(bool state) {
    bool ret = interrupt_state();
    if (state) {
        enable_interrupts();
    } else {
        disable_interrupts();
    }
    return ret;
}

static inline void halt(void) {
    asm ("hlt");
}

void cpu_init(struct limine_mp_info *mp_info);

#endif
