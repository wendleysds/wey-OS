#include <math.h>

double log2(double x) {
    double res;
    __asm__ volatile (
        "fld1\n\t"
        "fxch\n\t"
        "fyl2x"
        : "=t" (res)
        : "0" (x)
        : "st(1)"
    );
    return res;
}
