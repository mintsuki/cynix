#ifndef SYS__ISR_K_H_
#define SYS__ISR_K_H_

#include <stdint.h>

extern uint8_t isr_abort_vector;

void isr_init(void);

extern void *isr_thunks[];
extern void *isr_handler_table[];

#endif
