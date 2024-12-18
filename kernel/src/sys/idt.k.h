#ifndef SYS__IDT_K_H_
#define SYS__IDT_K_H_

#include <stdint.h>

void idt_init(void);
void idt_reload(void);
void idt_set_handler(uint8_t vector, void *isr_ptr, uint8_t ist, uint8_t flags);
void idt_set_ist(uint8_t vector, uint8_t ist);
uint8_t idt_allocate_vector(void);

#endif
