#include <stdlib.h>

div_t div(int numer, int denom) {
    div_t result;

    result.quot = numer / denom;
    result.rem  = numer % denom;

    return result;
}

ldiv_t ldiv(long numer, long denom) {
    ldiv_t result;
    div_t d = div((int)numer, (int)denom);
    result.quot = d.quot;
    result.rem  = d.rem;
    return result;
}