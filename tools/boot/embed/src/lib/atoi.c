#include <limits.h>

#define isdigit(c) (c >= '0' && c <= '9')
#define isspace(c) ((c)==' ' || (c)=='\t' || (c)=='\n' || (c)=='\r')

// Safe atoi
int atoi(const char *str){
	if (!str) return -1;

    const unsigned char *s = (const unsigned char *)str;
    while (*s && ((*s)==' ' || (*s)=='\t' || (*s)=='\n' || (*s)=='\r')) s++;

    int sign = 1;
    if (*s == '+' || *s == '-') {
        if (*s == '-') sign = -1;
        s++;
    }

    unsigned long limit_pos = (unsigned long)INT_MAX;
    unsigned long limit_neg = (unsigned long)INT_MAX + 1ULL;

    unsigned long acc = 0;
    int overflow = 0;
    while (*s >= '0' && *s <= '9') {
        unsigned digit = (unsigned)(*s - '0');
        unsigned long limit = (sign == 1) ? limit_pos : limit_neg;
        if (acc > (limit - digit) / 10) {
            overflow = 1;
            break;
        }
        acc = acc * 10 + digit;
        s++;
    }

    if (overflow) {
        return (sign == 1) ? INT_MAX : INT_MIN;
    }

    long val = (long)acc * sign;
    if (val > INT_MAX) return INT_MAX;
    if (val < INT_MIN) return INT_MIN;
    return (int)val;
}
