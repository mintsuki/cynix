#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <devices/fb/fb.k.h>

static bool fb_init_done = false;

size_t fb_count;
struct limine_framebuffer *fbs[FBS_MAX];

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request limine_framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

void fb_init(void) {
    if (fb_init_done) {
        return;
    }

    fb_count = 0;

    if (limine_framebuffer_request.response == NULL) {
        return;
    }

    for (; fb_count < limine_framebuffer_request.response->framebuffer_count; fb_count++) {
        if (fb_count == FBS_MAX) {
            break;
        }
        fbs[fb_count] = limine_framebuffer_request.response->framebuffers[fb_count];
    }

    fb_init_done = true;
}
