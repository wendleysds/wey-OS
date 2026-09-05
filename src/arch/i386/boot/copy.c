#include "boot.h"

void *memcpy_fromfs(void *dst_ds, uint32_t src_fs, size_t count) {
    void *d = dst_ds;
    asm volatile(
        "cld\n\t"
        "1:\n\t"
        "decl %2\n\t"
        "js 2f\n\t"
        "movb %%fs:(%1), %%al\n\t"
        "incl %1\n\t"
        "movb %%al, (%0)\n\t"
        "incl %0\n\t"
        "jmp 1b\n\t"
        "2:"
        : "+r" (d), "+r" (src_fs), "+r" (count)
        :
        : "eax", "memory", "cc"
    );
    return dst_ds;
}

void memcpy_tofs(uint32_t dst_fs, const void *src_ds, size_t count) {
    const void *s = src_ds;
    asm volatile(
        "cld\n\t"
        "1:\n\t"
        "decl %2\n\t"
        "js 2f\n\t"
        "movb (%0), %%al\n\t"
        "incl %0\n\t"
        "movb %%al, %%fs:(%1)\n\t"
        "incl %1\n\t"
        "jmp 1b\n\t"
        "2:"
        : "+r" (s), "+r" (dst_fs), "+r" (count)
        :
        : "eax", "memory", "cc"
    );
}

void *memset_fs(uint32_t dst_fs, int c, size_t count) {
    uint32_t d = dst_fs;
    asm volatile(
        "cld\n\t"
        "1:\n\t"
        "decl %2\n\t"
        "js 2f\n\t"
        "movb %b3, %%fs:(%0)\n\t"
        "incl %0\n\t"
        "jmp 1b\n\t"
        "2:"
        : "+r" (d), "+r" (count)
        : "q" (c)
        : "memory", "cc"
    );
    return (void *)dst_fs;
}

int memcmp_fs(uint32_t s1_fs, const void *s2_ds, size_t count) {
    const uint8_t *p2 = (const uint8_t *)s2_ds;
    uint8_t u1, u2;
    while (count--) {
        asm volatile("movb %%fs:(%1), %0" : "=q" (u1) : "r" (s1_fs));
        u2 = *p2;
        if (u1 != u2)
            return (int)u1 - (int)u2;
        s1_fs++;
        p2++;
    }
    return 0;
}