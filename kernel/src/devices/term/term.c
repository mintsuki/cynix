#include <stddef.h>
#include <stdbool.h>
#include <flanterm/backends/fb.h>
#include <devices/term/term.k.h>
#include <devices/fb/fb.k.h>
#include <lib/spinlock.k.h>
#include <lib/alloc.k.h>
#include <sys/cpu.k.h>

#define TERMS_MAX 16

#define TERM_INIT_NOT_DONE 0
#define TERM_INIT_DEBUG_DONE 1
#define TERM_INIT_NORMAL_DONE 2
static int term_init_done = TERM_INIT_NOT_DONE;

struct term {
    struct spinlock spinlock;
    struct flanterm_context *ft_ctx;
};

static size_t term_count = 0;
static struct term terms[TERMS_MAX];

void term_init(bool debug) {
    if (debug == true) {
        if (term_init_done != TERM_INIT_NOT_DONE) {
            return;
        }
    } else {
        if (term_init_done == TERM_INIT_NORMAL_DONE) {
            return;
        }
    }

    fb_init();

    // Check if previous debug terminal started and deinit it.
    if (term_count != 0) {
        terms[0].ft_ctx->deinit(terms[0].ft_ctx, NULL);
        term_count = 0;
    }

    size_t terms_limit = debug ? 1 : TERMS_MAX;
    void *malloc_func = debug ? NULL : alloc;
    void *free_func = debug ? NULL : free;

    for (; term_count < fb_count; term_count++) {
        if (term_count == terms_limit) {
            break;
        }

        terms[term_count].spinlock = SPINLOCK_INITIALISER;

        struct limine_framebuffer *fb = fbs[term_count];

        terms[term_count].ft_ctx = flanterm_fb_init(
            malloc_func, free_func,
            fb->address, fb->width, fb->height, fb->pitch,
            fb->red_mask_size, fb->red_mask_shift,
            fb->green_mask_size, fb->green_mask_shift,
            fb->blue_mask_size, fb->blue_mask_shift,
            NULL,
            NULL, NULL,
            NULL, NULL,
            NULL, NULL,
            NULL, 0, 0, 1,
            1, 1,
            0
        );
    }

    term_init_done = debug ? TERM_INIT_DEBUG_DONE : TERM_INIT_NORMAL_DONE;
}

void term_write(size_t terminal, const void *data, size_t len) {
    if (term_init_done == TERM_INIT_NOT_DONE) {
        return;
    }

    int old_int_state = interrupt_toggle(false);

    spinlock_acquire(&terms[terminal].spinlock);
    flanterm_write(terms[terminal].ft_ctx, data, len);
    spinlock_release(&terms[terminal].spinlock);

    interrupt_toggle(old_int_state);
}
