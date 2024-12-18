#ifndef DEVICES__TERM__TERM_K_H_
#define DEVICES__TERM__TERM_K_H_

#include <stddef.h>
#include <stdbool.h>

void term_init(bool debug);
void term_write(size_t terminal, const void *data, size_t len);

#endif
