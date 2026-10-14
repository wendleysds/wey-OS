#include <math.h>

float logf(float x) {
    float res;
    __asm__ volatile (
        "fldln2\n\t"
        "fxch\n\t"
        "fyl2x"
        : "=t" (res)
        : "0" ((double)x)
        : "st(1)"
    );
    return res;
}
