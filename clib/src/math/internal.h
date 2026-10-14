#ifndef _MATH_INTERNAL_H
#define _MATH_INTERNAL_H

/**
 * @brief Computes 2^w using x87 f2xm1 and fscale.
 *
 * For any real w, decomposes into integer i = round(w) and fraction f = w - i.
 * Since |f| <= 0.5, f2xm1 is valid (requires [-1.0, 1.0]).
 * Then computes ( (2^f - 1) + 1 ) * 2^i = 2^w.
 */
static inline double __fpu_pow2(double w) {
    double res;
    __asm__ volatile (
        "fld %%st(0)\n\t"
        "frndint\n\t"
        "fxch %%st(1)\n\t"
        "fsub %%st(1), %%st(0)\n\t"
        "f2xm1\n\t"
        "fld1\n\t"
        "faddp\n\t"
        "fscale\n\t"
        "fstp %%st(1)"
        : "=t" (res)
        : "0" (w)
    );
    return res;
}

#endif /* _MATH_INTERNAL_H */
