#ifndef SYS__HPET_K_H_
#define SYS__HPET_K_H_

#include <stdint.h>

extern uint64_t hpet_frequency;

uint64_t hpet_read_counter(void);
void hpet_init(void);

#endif
