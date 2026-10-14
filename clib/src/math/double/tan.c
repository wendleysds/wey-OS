#include <math.h>

double tan(double x) {
    double res;
    __asm__ volatile (
        "fptan\n\t"
        "fstp %%st(0)"
        : "=t" (res) : "0" (x)
    );
    return res;
}
