#include <math.h>

double ldexp(double x, int exp) {
    double res;
    double d_exp = (double)exp;
    __asm__ volatile ("fscale" : "=t" (res) : "0" (x), "u" (d_exp));
    return res;
}
