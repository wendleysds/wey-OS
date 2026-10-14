#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

float strtof(const char *str, char **endptr) {
    const char *s = str;
    while (isspace((unsigned char)*s)) {
        s++;
    }

    bool neg = false;
    if (*s == '-') {
        neg = true;
        s++;
    } else if (*s == '+') {
        s++;
    }

    if ((s[0] == 'i' || s[0] == 'I') &&
        (s[1] == 'n' || s[1] == 'N') &&
        (s[2] == 'f' || s[2] == 'F')) {
        s += 3;
        if ((s[0] == 'i' || s[0] == 'I') &&
            (s[1] == 'n' || s[1] == 'N') &&
            (s[2] == 'i' || s[2] == 'I') &&
            (s[3] == 't' || s[3] == 'T') &&
            (s[4] == 'y' || s[4] == 'Y')) {
            s += 5;
        }
        if (endptr) *endptr = (char *)s;
        return neg ? -INFINITY : INFINITY;
    }

    if ((s[0] == 'n' || s[0] == 'N') &&
        (s[1] == 'a' || s[1] == 'A') &&
        (s[2] == 'n' || s[2] == 'N')) {
        s += 3;
        if (endptr) *endptr = (char *)s;
        return neg ? -NAN : NAN;
    }

    double value = 0.0;
    bool has_digits = false;

    while (isdigit((unsigned char)*s)) {
        has_digits = true;
        value = value * 10.0 + (*s - '0');
        s++;
    }

    if (*s == '.') {
        s++;
        double frac = 0.1;
        while (isdigit((unsigned char)*s)) {
            has_digits = true;
            value += (*s - '0') * frac;
            frac *= 0.1;
            s++;
        }
    }

    if (!has_digits) {
        if (endptr) *endptr = (char *)str;
        return 0.0f;
    }

    if (*s == 'e' || *s == 'E') {
        const char *exp_start = s;
        s++;
        bool exp_neg = false;
        if (*s == '-') {
            exp_neg = true;
            s++;
        } else if (*s == '+') {
            s++;
        }

        if (isdigit((unsigned char)*s)) {
            int exp = 0;
            while (isdigit((unsigned char)*s)) {
                if (exp < 1000) {
                    exp = exp * 10 + (*s - '0');
                }
                s++;
            }
            double factor = 1.0;
            double base = 10.0;
            int e = exp;
            while (e > 0) {
                if (e & 1) factor *= base;
                base *= base;
                e >>= 1;
            }
            if (exp_neg) {
                value /= factor;
            } else {
                value *= factor;
            }
        } else {
            s = exp_start;
        }
    }

    if (endptr) {
        *endptr = (char *)s;
    }

    return (float)(neg ? -value : value);
}
