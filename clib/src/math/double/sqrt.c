#include <math.h>

double sqrt(double x) {
    double res;
    __asm__ volatile ("fsqrt" : "=t" (res) : "0" (x));
    return res;
}
