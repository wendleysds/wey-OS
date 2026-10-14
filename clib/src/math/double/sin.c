#include <math.h>

double sin(double x) {
    double res;
    __asm__ volatile ("fsin" : "=t" (res) : "0" (x));
    return res;
}
