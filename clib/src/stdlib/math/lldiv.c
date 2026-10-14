#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdint.h>

extern uint64_t do_div_64(uint64_t *n, uint64_t base);

lldiv_t lldiv(long long numer, long long denom) {
    lldiv_t result;

    bool quot_neg = (numer < 0) ^ (denom < 0);
    bool rem_neg  = (numer < 0);

    uint64_t u_numer = (numer < 0) ? -(uint64_t)numer : (uint64_t)numer;
    uint64_t u_denom = (denom < 0) ? -(uint64_t)denom : (uint64_t)denom;

    uint64_t u_rem = do_div_64(&u_numer, u_denom);

    result.quot = quot_neg ? -(long long)u_numer : (long long)u_numer;
    result.rem  = rem_neg  ? -(long long)u_rem   : (long long)u_rem;

    return result;
}