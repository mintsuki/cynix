#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1
#include <lib/nanoprintf.h>

#include <stddef.h>
#include <stdarg.h>
#include <lib/printf.k.h>
#include <devices/term/term.k.h>
#include <lib/spinlock.k.h>
#include <sys/port.k.h>
#include <sys/cpu.k.h>

static struct spinlock printf_lock = SPINLOCK_INITIALISER;

static void debug_putchar(int c, void *ctx) {
    (void)ctx;
    term_write(0, &c, 1);
#ifdef DEBUG
    outb(0xe9, c);
#endif
}

int printf(const char *fmt, ...) {
    va_list l;
    va_start(l, fmt);
    int ret = vprintf(fmt, l);
    va_end(l);
    return ret;
}

int vprintf(const char *fmt, va_list l) {
    term_init(true);

    int old_int_state = interrupt_toggle(false);

    spinlock_acquire(&printf_lock);
    int ret = npf_vpprintf(debug_putchar, NULL, fmt, l);
    spinlock_release(&printf_lock);

    interrupt_toggle(old_int_state);

    return ret;
}

int snprintf(char *buf, size_t bufsz, const char *fmt, ...) {
    va_list l;
    va_start(l, fmt);

    int ret = npf_vsnprintf(buf, bufsz, fmt, l);

    va_end(l);
    return ret;
}
