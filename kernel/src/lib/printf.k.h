#ifndef LIB__PRINTF_K_H_
#define LIB__PRINTF_K_H_

#include <stddef.h>
#include <stdarg.h>

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list l);
int snprintf(char *buf, size_t bufsz, const char *fmt, ...);

#endif
