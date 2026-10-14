#include <math.h>

double atan2(double y, double x) {
    double res;
    __asm__ volatile (
        "fpatan"
        : "=t" (res)
        : "0" (x), "u" (y)
        : "st(1)"
    );
    return res;
}
