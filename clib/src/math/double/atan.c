#include <math.h>

double atan(double x) {
    double res;
    __asm__ volatile (
        "fld1\n\t"
        "fpatan"
        : "=t" (res)
        : "0" (x)
        : "st(1)"
    );
    return res;
}
