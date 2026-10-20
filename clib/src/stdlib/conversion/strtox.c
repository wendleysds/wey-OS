#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint64_t do_div_64(uint64_t *n, uint64_t base);

static unsigned long long strtox_impl(const char *nptr, char **endptr, int base, bool is_signed, unsigned long long max_val, long long min_val) {
    const char *s = nptr;
    unsigned long long acc = 0;
    int c;
    bool neg = false;
    bool any_digits = false;
    bool overflow = false;

    while (isspace((unsigned char)*s)) {
        s++;
    }

    if (*s == '-') {
        neg = true;
        s++;
    } else if (*s == '+') {
        s++;
    }

    if (base == 0) {
        if (*s == '0') {
            s++;
            if (*s == 'x' || *s == 'X') {
                s++;
                base = 16;
            } else {
                base = 8;
                any_digits = true;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
        }
    }

    if (base < 2 || base > 36) {
        errno = EINVAL;
        if (endptr) *endptr = (char *)nptr;
        return 0;
    }

    /* Ajuste para calcular cutoff e cutlim simultaneamente via lldiv / do_div_64 */
    unsigned long long cutoff;
    unsigned long long cutlim;

    unsigned long long limit = neg ? ((unsigned long long)-(min_val)) : max_val;

    /* Como limit e base são não-assinados (uint64_t), usamos do_div_64 
       para evitar casting assinado com lldiv caso limit > LLONG_MAX */
    uint64_t tmp_limit = limit;
    cutlim = do_div_64(&tmp_limit, (uint32_t)base);
    cutoff = tmp_limit;

    for (;; s++) {
        c = (unsigned char)*s;
        if (isdigit(c)) {
            c -= '0';
        } else if (isalpha(c)) {
            c = toupper(c) - 'A' + 10;
        } else {
            break;
        }

        if (c >= base) {
            break;
        }

        any_digits = true;

        if (overflow || acc > cutoff || (acc == cutoff && (unsigned long long)c > cutlim)) {
            overflow = true;
        } else {
            acc = acc * base + c;
        }
    }

    if (!any_digits) {
        s = nptr;
    } else if (endptr) {
        *endptr = (char *)s;
    }

    if (overflow) {
        errno = ERANGE;
        if (is_signed) {
            return neg ? (unsigned long long)min_val : max_val;
        } else {
            return max_val;
        }
    }

    if (neg) {
        return (unsigned long long)(-(long long)acc);
    }

    return acc;
}

long strtol(const char *str, char **endptr, int base) {
    return (long)strtox_impl(str, endptr, base, true, LONG_MAX, LONG_MIN);
}

unsigned long strtoul(const char *str, char **endptr, int base) {
    return (unsigned long)strtox_impl(str, endptr, base, false, ULONG_MAX, 0);
}

long long strtoll(const char *str, char **endptr, int base) {
    return (long long)strtox_impl(str, endptr, base, true, LLONG_MAX, LLONG_MIN);
}

unsigned long long strtoull(const char *str, char **endptr, int base) {
    return strtox_impl(str, endptr, base, false, ULLONG_MAX, 0);
}

int atoi(const char *str) {
    return (int)strtol(str, NULL, 10);
}

long atol(const char *str) {
    return strtol(str, NULL, 10);
}

long long atoll(const char *str) {
    return strtoll(str, NULL, 10);
}