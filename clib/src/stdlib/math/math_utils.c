#include <stdint.h>

uint32_t do_div_32(uint64_t *n, uint32_t base) {
    uint32_t rem;
    uint32_t high = (uint32_t)(*n >> 32);
    uint32_t low = (uint32_t)(*n);

    if (high == 0) {
        asm("divl %4"
            : "=a"(low), "=d"(rem)
            : "a"(low), "d"(0), "r"(base));
        *n = low;
    } else {
        asm("divl %4"
            : "=a"(high), "=d"(rem)
            : "a"(high), "d"(0), "r"(base));

        asm("divl %4"
            : "=a"(low), "=d"(rem)
            : "a"(low), "d"(rem), "r"(base));

        *n = ((uint64_t)high << 32) | low;
    }

    return rem;
}

uint64_t do_div_64(uint64_t *n, uint64_t base) {
    if (base == 0) return 0;

    if (base <= UINT32_MAX) {
        return do_div_32(n, (uint32_t)base);
    }

    uint64_t dividend = *n;

    if (dividend < base) {
        *n = 0;
        return dividend;
    }

    int shift = __builtin_clzll(base) - __builtin_clzll(dividend);
    uint64_t current_base = base << shift;
    uint64_t quotient = 0;

    for (int i = 0; i <= shift; i++) {
        quotient <<= 1;
        if (dividend >= current_base) {
            dividend -= current_base;
            quotient |= 1;
        }
        current_base >>= 1;
    }

    *n = quotient;
    return dividend;
}