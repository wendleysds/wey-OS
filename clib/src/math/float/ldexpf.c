#include <math.h>

float ldexpf(float x, int exp) {
    float res;
    double d_exp = (double)exp;
    __asm__ volatile ("fscale" : "=t" (res) : "0" ((double)x), "u" (d_exp));
    return res;
}
