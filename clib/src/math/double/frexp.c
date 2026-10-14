#include <math.h>
#include <stdint.h>

double frexp(double x, int *exp) {
    union {
        double d;
        uint64_t u;
    } conv;
    conv.d = x;
    uint64_t u = conv.u;
    int e = (int)((u >> 52) & 0x7ff);

    if (e == 0) {
        if ((u << 1) == 0) {
            if (exp) *exp = 0;
            return x;
        }
        conv.d = x * 4503599627370496.0; // 2^52
        u = conv.u;
        e = (int)((u >> 52) & 0x7ff) - 52;
    } else if (e == 0x7ff) {
        if (exp) *exp = 0;
        return x;
    }

    if (exp) *exp = e - 1022;
    conv.u = (u & 0x800fffffffffffffULL) | ((uint64_t)1022 << 52);
    return conv.d;
}
