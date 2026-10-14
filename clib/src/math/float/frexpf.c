#include <math.h>
#include <stdint.h>

float frexpf(float x, int *exp) {
    union {
        float f;
        uint32_t u;
    } conv;
    conv.f = x;
    uint32_t u = conv.u;
    int e = (int)((u >> 23) & 0xff);

    if (e == 0) {
        if ((u << 1) == 0) {
            if (exp) *exp = 0;
            return x;
        }
        conv.f = x * 16777216.0f; // 2^24
        u = conv.u;
        e = (int)((u >> 23) & 0xff) - 24;
    } else if (e == 0xff) {
        if (exp) *exp = 0;
        return x;
    }

    if (exp) *exp = e - 126;
    conv.u = (u & 0x807fffff) | ((uint32_t)126 << 23);
    return conv.f;
}
