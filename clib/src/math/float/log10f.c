#include <math.h>

float log10f(float x) {
    float res;
    __asm__ volatile (
        "fldlg2\n\t"
        "fxch\n\t"
        "fyl2x"
        : "=t" (res)
        : "0" ((double)x)
        : "st(1)"
    );
    return res;
}
