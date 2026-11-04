#include <stdint.h>
#include "../math_internal.h"

int64_t __moddi3(int64_t a, int64_t b)
{
    uint64_t ua = (a < 0) ? -(uint64_t)a : (uint64_t)a;
    uint64_t ub = (b < 0) ? -(uint64_t)b : (uint64_t)b;

    uint64_t rem;
    udivmod64(ua, ub, &rem);

    return (a < 0) ? -(int64_t)rem : (int64_t)rem;
}
