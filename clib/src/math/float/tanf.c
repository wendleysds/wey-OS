#include <math.h>

float tanf(float x) {
    float res;
    __asm__ volatile (
        "fptan\n\t"
        "fstp %%st(0)"
        : "=t" (res) : "0" ((double)x)
    );
    return res;
}
