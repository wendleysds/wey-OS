#include <math.h>

double cos(double x) {
    double res;
    __asm__ volatile ("fcos" : "=t" (res) : "0" (x));
    return res;
}
