#ifndef SYS__GDT_K_H_
#define SYS__GDT_K_H_

#ifndef __ASSEMBLER__

#include <sys/cpu.k.h>

#endif

#define GDT_KERNEL_CODE_SEL 0x08
#define GDT_KERNEL_DATA_SEL 0x10
#define GDT_USER_CODE_SEL 0x1b
#define GDT_USER_DATA_SEL 0x23
#define GDT_TSS_SEL 0x28

#ifndef __ASSEMBLER__

void gdt_init(void);
void gdt_reload(void);
void gdt_load_tss(struct cpu_tss *tss);

#endif

#endif
