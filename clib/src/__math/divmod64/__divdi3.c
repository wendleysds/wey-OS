#include <stdint.h>
#include "../math_internal.h"

int64_t __divdi3(int64_t a, int64_t b)
{
    uint64_t ua = (a < 0) ? -(uint64_t)a : (uint64_t)a;
    uint64_t ub = (b < 0) ? -(uint64_t)b : (uint64_t)b;

    uint64_t q = udivmod64(ua, ub, (void *)0);

    return ((a ^ b) < 0) ? -(int64_t)q : (int64_t)q;
}
