#include <math.h>
#include "../internal.h"

static inline double fpu_yl2x(double x, double y) {
    double res;
    __asm__ volatile ("fyl2x" : "=t" (res) : "0" ((double)x), "u" ((double)y) : "st(1)");
    return res;
}

float powf(float base, float exponent) {
    if (exponent == 0.0f) return 1.0f;
    if (base == 1.0f) return 1.0f;
    if (base == 0.0f) {
        if (exponent < 0.0f) return 1.0f / 0.0f;
        return 0.0f;
    }
    if (base < 0.0f) {
        long long n = (long long)exponent;
        if ((float)n == exponent) {
            double res = __fpu_pow2(fpu_yl2x((double)-base, (double)exponent));
            return (float)((n & 1) ? -res : res);
        }
        return 0.0f / 0.0f; // NaN
    }
    return (float)__fpu_pow2(fpu_yl2x((double)base, (double)exponent));
}
