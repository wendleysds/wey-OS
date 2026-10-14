#include <stddef.h>
#include <stdbool.h>

static const char* const digits = "0123456789abcdefghijklmnopqrstuvwxyz";

static char *xtoa_impl(unsigned long long val, char *str, size_t size, int base, bool is_negative) {
    if (!str || size == 0) return NULL;
    if (base < 2 || base > 36) {
        if (size > 0) str[0] = '\0';
        return NULL;
    }

    char buf[65];
    int pos = sizeof(buf) - 1;
    buf[pos] = '\0';

    do {
        buf[--pos] = digits[val % (unsigned int)base];
        val /= (unsigned int)base;
    } while (val > 0);

    if (is_negative) {
        buf[--pos] = '-';
    }

    size_t len = sizeof(buf) - 1 - pos;

    if (len >= size) {
        str[0] = '\0';
        return NULL;
    }
    for (size_t i = 0; i < len; i++) {
        str[i] = buf[pos + i];
    }
    str[len] = '\0';

    return str;
}

char *utoa(unsigned int val, char *str, size_t size, int base) {
    return xtoa_impl(val, str, size, base, false);
}

char *itoa(int val, char *str, size_t size, int base) {
    bool is_neg = false;
    unsigned long long uval;

    if (base == 10 && val < 0) {
        is_neg = true;
        uval = (unsigned int)(-(val + 1)) + 1U;
    } else {
        uval = (unsigned int)val;
    }

    return xtoa_impl(uval, str, size, base, is_neg);
}

char *lltoa(long long val, char *str, size_t size, int base) {
    bool is_neg = false;
    unsigned long long uval;

    if (base == 10 && val < 0) {
        is_neg = true;
        uval = (unsigned long long)(-(val + 1)) + 1ULL;
    } else {
        uval = (unsigned long long)val;
    }

    return xtoa_impl(uval, str, size, base, is_neg);
}

char *ulltoa(unsigned long long val, char *str, size_t size, int base) {
    return xtoa_impl(val, str, size, base, false);
}