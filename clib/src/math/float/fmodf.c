#include <math.h>

float fmodf(float x, float y) {
    float res;
    __asm__ volatile (
        "1:\n\t"
        "fprem\n\t"
        "fnstsw %%ax\n\t"
        "sahf\n\t"
        "jp 1b"
        : "=t" (res)
        : "0" ((double)x), "u" ((double)y)
        : "ax", "cc"
    );
    return res;
}
