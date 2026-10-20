#include <stdlib.h>

div_t div(int numer, int denom) {
    div_t result;

#if defined(__i386__) || defined(__i686__)
    asm("cltd\n\t"
        "idivl %2"
        : "=a"(result.quot), "=d"(result.rem)
        : "r"(denom), "a"(numer)
        : "cc");
#else
    result.quot = numer / denom;
    result.rem  = numer % denom;
#endif

    return result;
}

ldiv_t ldiv(long numer, long denom) {
    ldiv_t result;
    div_t d = div((int)numer, (int)denom);
    result.quot = d.quot;
    result.rem  = d.rem;
    return result;
}