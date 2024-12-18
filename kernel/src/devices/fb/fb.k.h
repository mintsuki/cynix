#ifndef DEVICES__FB__FB_K_H_
#define DEVICES__FB__FB_K_H_

#include <stddef.h>
#include <limine.h>

#define FBS_MAX 16

extern size_t fb_count;
extern struct limine_framebuffer *fbs[FBS_MAX];

void fb_init(void);

#endif
