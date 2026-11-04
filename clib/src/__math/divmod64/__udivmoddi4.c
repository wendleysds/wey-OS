#include <stdint.h>
#include "../math_internal.h"

uint64_t __udivmoddi4(uint64_t a, uint64_t b, uint64_t *rem)
{
    return udivmod64(a, b, rem);
}

