#include <math.h>

float atanf(float x) {
    float res;
    __asm__ volatile (
        "fld1\n\t"
        "fpatan"
        : "=t" (res)
        : "0" ((double)x)
        : "st(1)"
    );
    return res;
}
