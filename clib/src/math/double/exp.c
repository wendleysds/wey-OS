#include <math.h>
#include "../internal.h"

double exp(double x) {
    double l2e;
    __asm__ volatile ("fldl2e" : "=t" (l2e));
    return __fpu_pow2(x * l2e);
}
