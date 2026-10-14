#include <math.h>
#include "../internal.h"

float expf(float x) {
    double l2e;
    __asm__ volatile ("fldl2e" : "=t" (l2e));
    return (float)__fpu_pow2((double)x * l2e);
}
