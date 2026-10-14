#include <math.h>

double ceil(double x) {
    unsigned short old_cw, new_cw;
    double res;
    __asm__ volatile ("fnstcw %0" : "=m" (old_cw));
    new_cw = (old_cw & ~0x0c00) | 0x0800; /* RC = 10: round up */
    __asm__ volatile ("fldcw %0" : : "m" (new_cw));
    __asm__ volatile ("frndint" : "=t" (res) : "0" (x));
    __asm__ volatile ("fldcw %0" : : "m" (old_cw));
    return res;
}
