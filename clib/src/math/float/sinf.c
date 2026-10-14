#include <math.h>

float sinf(float x) {
    float res;
    __asm__ volatile ("fsin" : "=t" (res) : "0" ((double)x));
    return res;
}
