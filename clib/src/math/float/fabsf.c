#include <math.h>

float fabsf(float x) {
    float res;
    __asm__ volatile ("fabs" : "=t" (res) : "0" ((double)x));
    return res;
}
