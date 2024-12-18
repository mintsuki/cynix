#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limine.h>
#include <sys/mp.k.h>
#include <sys/cpu.k.h>
#include <lib/printf.k.h>
#include <lib/alloc.k.h>
#include <lib/vector.k.h>

__attribute__((used, section(".limine_requests")))
volatile struct limine_mp_request limine_mp_request = {
    .id = LIMINE_MP_REQUEST,
    .revision = 0,
    .flags = LIMINE_MP_X2APIC
};

bool mp_init_done = false;

void mp_init(void) {
    struct limine_mp_response *mp_response = limine_mp_request.response;

    printf("mp: BSP LAPIC ID:    %x\n", mp_response->bsp_lapic_id);
    printf("mp: Total CPU count: %zu\n", mp_response->cpu_count);
    printf("mp: Using x2APIC:    %s\n", mp_response->flags & LIMINE_MP_X2APIC ? "yes" : "no");

    struct limine_mp_info **mp_info_array = mp_response->cpus;

    uint32_t bsp_lapic_id = mp_response->bsp_lapic_id;

    for (size_t i = 0; i < mp_response->cpu_count; i++) {
        struct cpu_local *cpu_local = alloc(sizeof(struct cpu_local));
        VECTOR_PUSH_BACK(&cpu_locals, cpu_local);

        struct limine_mp_info *mp_info = mp_info_array[i];

        mp_info->extra_argument = (uint64_t)cpu_local;
        cpu_local->cpu_number = i;

        if (mp_info->lapic_id == bsp_lapic_id) {
            cpu_local->is_bsp = true;
            cpu_init(mp_info);
            continue;
        }

        mp_info->goto_address = cpu_init;

        while (!cpu_local->online) {
            asm ("pause");
        }
    }

    mp_init_done = true;

    printf("mp: All CPUs online!\n");
}
