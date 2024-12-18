#ifndef LIB__MISC_K_H_
#define LIB__MISC_K_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#define MIN(A, B) ({ \
    __auto_type MIN_a = A; \
    __auto_type MIN_b = B; \
    MIN_a < MIN_b ? MIN_a : MIN_b; \
})

#define MAX(A, B) ({ \
    __auto_type MAX_a = A; \
    __auto_type MAX_b = B; \
    MAX_a > MAX_b ? MAX_a : MAX_b; \
})

#define DIV_ROUNDUP(VALUE, DIV) ({ \
    __auto_type DIV_ROUNDUP_value = VALUE; \
    __auto_type DIV_ROUNDUP_div = DIV; \
    (DIV_ROUNDUP_value + (DIV_ROUNDUP_div - 1)) / DIV_ROUNDUP_div; \
})

#define ALIGN_UP(VALUE, ALIGN) ({ \
    __auto_type ALIGN_UP_value = VALUE; \
    __auto_type ALIGN_UP_align = ALIGN; \
    DIV_ROUNDUP(ALIGN_UP_value, ALIGN_UP_align) * ALIGN_UP_align; \
})

#define ALIGN_DOWN(VALUE, ALIGN) ({ \
    __auto_type ALIGN_DOWN_value = VALUE; \
    __auto_type ALIGN_DOWN_align = ALIGN; \
    (ALIGN_DOWN_value / ALIGN_DOWN_align) * ALIGN_DOWN_align; \
})

#define SIZEOF_ARRAY(array) (sizeof(array) / sizeof(array[0]))

static inline bool bit_test(void *bitmap, size_t bit) {
    uint8_t *bitmap_u8 = bitmap;
    return bitmap_u8[bit / 8] & (1 << (bit % 8));
}

static inline void bit_set(void *bitmap, size_t bit) {
    uint8_t *bitmap_u8 = bitmap;
    bitmap_u8[bit / 8] |= (1 << (bit % 8));
}

static inline void bit_reset(void *bitmap, size_t bit) {
    uint8_t *bitmap_u8 = bitmap;
    bitmap_u8[bit / 8] &= ~(1 << (bit % 8));
}

#define STR(V) STR_1(V)
#define STR_1(V) #V

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

typedef char symbol[];

extern volatile struct limine_executable_file_request limine_executable_file_request;

#endif
