#include <string.h>

size_t strcspn(const char *str, const char *reject) {
    size_t i = 0;
    while (str[i]) {
        const char *r = reject;
        while (*r) {
            if (str[i] == *r) return i;
            r++;
        }
        i++;
    }
    return i;
}
