#include <string.h>

int strncmp(const char *a, const char *b, size_t n){
    while (n && *a && *a == *b) { a++; b++; n--; }
    return n ? (unsigned char)*a - (unsigned char)*b : 0;
}