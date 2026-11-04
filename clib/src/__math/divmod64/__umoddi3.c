#include <stdint.h>
#include "../math_internal.h"

uint64_t __umoddi3(uint64_t a, uint64_t b)
{
    uint64_t rem;
    udivmod64(a, b, &rem);
    return rem;
}
