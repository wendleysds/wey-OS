#include <math.h>

float sqrtf(float x) {
    float res;
    __asm__ volatile ("fsqrt" : "=t" (res) : "0" ((double)x));
    return res;
}
