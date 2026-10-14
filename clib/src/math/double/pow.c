#include <math.h>
#include "../internal.h"

static inline double fpu_yl2x(double x, double y) {
    double res;
    __asm__ volatile ("fyl2x" : "=t" (res) : "0" (x), "u" (y) : "st(1)");
    return res;
}

double pow(double base, double exponent) {
    if (exponent == 0.0) return 1.0;
    if (base == 1.0) return 1.0;
    if (base == 0.0) {
        if (exponent < 0.0) return 1.0 / 0.0;
        return 0.0;
    }
    if (base < 0.0) {
        long long n = (long long)exponent;
        if ((double)n == exponent) {
            double res = __fpu_pow2(fpu_yl2x(-base, exponent));
            return (n & 1) ? -res : res;
        }
        return 0.0 / 0.0; // NaN
    }
    return __fpu_pow2(fpu_yl2x(base, exponent));
}
