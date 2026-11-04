#include <stdint.h>
#include "../math_internal.h"

uint64_t __udivdi3(uint64_t a, uint64_t b)
{
    return udivmod64(a, b, (void *)0);
}
