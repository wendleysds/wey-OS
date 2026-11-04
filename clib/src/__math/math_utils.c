#include <stdint.h>
#include "math_internal.h"

static inline uint64_t udivmod64_32(uint64_t n, uint32_t base, uint32_t *rem)
{
    uint32_t n_hi = (uint32_t)(n >> 32);
    uint32_t n_lo = (uint32_t)n;
    uint32_t q_hi = 0;
    uint32_t q_lo;
    uint32_t r;

    if (n_hi >= base) {
        __asm__("divl %4"
                : "=a"(q_hi), "=d"(r)
                : "a"(n_hi), "d"(0), "rm"(base));
        __asm__("divl %4"
                : "=a"(q_lo), "=d"(r)
                : "a"(n_lo), "d"(r), "rm"(base));
    } else {
        __asm__("divl %4"
                : "=a"(q_lo), "=d"(r)
                : "a"(n_lo), "d"(n_hi), "rm"(base));
    }

    if (rem)
        *rem = r;

    return ((uint64_t)q_hi << 32) | q_lo;
}

uint64_t udivmod64(uint64_t a, uint64_t b, uint64_t *rem)
{
    if (a < b) {
        if (rem) *rem = a;
        return 0;
    }

    if (b == 1) {
        if (rem) *rem = 0;
        return a;
    }

    uint32_t b_hi = (uint32_t)(b >> 32);

    if (b_hi == 0) {
        uint32_t r;
        uint64_t q = udivmod64_32(a, (uint32_t)b, &r);
        if (rem) *rem = r;
        return q;
    }

    uint32_t s = __builtin_clz(b_hi);

    if (s == 0) {
        if (rem) *rem = a - b;
        return 1;
    }

    uint64_t b_norm = b << s;
    uint32_t v1 = (uint32_t)(b_norm >> 32);
    uint32_t v0 = (uint32_t)b_norm;

    uint32_t u2 = (uint32_t)(a >> (64 - s));
    uint64_t a_norm = a << s;
    uint32_t u1 = (uint32_t)(a_norm >> 32);
    uint32_t u0 = (uint32_t)a_norm;

    uint32_t q_hat, r_hat;
    if (u2 >= v1) {
        q_hat = 0xFFFFFFFF;
        r_hat = u1 + v1;
    } else {
        __asm__("divl %4"
                : "=a"(q_hat), "=d"(r_hat)
                : "a"(u1), "d"(u2), "rm"(v1));
    }

    while ((uint64_t)q_hat * v0 > (((uint64_t)r_hat << 32) | u0)) {
        q_hat--;
        r_hat += v1;
        if (r_hat < v1)
            break;
    }

    uint64_t prod = (uint64_t)q_hat * b;
    uint64_t rem_val;

    if (a < prod) {
        q_hat--;
        rem_val = (a + b) - prod;
    } else {
        rem_val = a - prod;
    }

    if (rem)
        *rem = rem_val;

    return q_hat;
}

uint32_t do_div_32(uint64_t *n, uint32_t base)
{
    if (base == 0)
        return 0;

    uint32_t rem;
    *n = udivmod64_32(*n, base, &rem);
    return rem;
}

uint64_t do_div_64(uint64_t *n, uint64_t base)
{
    uint64_t rem;
    *n = udivmod64(*n, base, &rem);
    return rem;
}
