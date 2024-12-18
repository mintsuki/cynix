#ifndef SYS__MP_K_H_
#define SYS__MP_K_H_

#include <stdbool.h>
#include <limine.h>

extern volatile struct limine_mp_request limine_mp_request;
extern bool mp_init_done;

void mp_init(void);

#endif
