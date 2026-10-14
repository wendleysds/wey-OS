#include <math.h>

double fmod(double x, double y) {
    double res;
    __asm__ volatile (
        "1:\n\t"
        "fprem\n\t"
        "fnstsw %%ax\n\t"
        "sahf\n\t"
        "jp 1b"
        : "=t" (res)
        : "0" (x), "u" (y)
        : "ax", "cc"
    );
    return res;
}
