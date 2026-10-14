#include <math.h>

float atan2f(float y, float x) {
    float res;
    __asm__ volatile (
        "fpatan"
        : "=t" (res)
        : "0" ((double)x), "u" ((double)y)
        : "st(1)"
    );
    return res;
}
