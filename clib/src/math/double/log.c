#include <math.h>

double log(double x) {
    double res;
    __asm__ volatile (
        "fldln2\n\t"
        "fxch\n\t"
        "fyl2x"
        : "=t" (res)
        : "0" (x)
        : "st(1)"
    );
    return res;
}
