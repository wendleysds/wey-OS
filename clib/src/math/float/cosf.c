#include <math.h>

float cosf(float x) {
    float res;
    __asm__ volatile ("fcos" : "=t" (res) : "0" ((double)x));
    return res;
}
