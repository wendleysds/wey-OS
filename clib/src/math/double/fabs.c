#include <math.h>

double fabs(double x) {
    double res;
    __asm__ volatile ("fabs" : "=t" (res) : "0" (x));
    return res;
}
