#include <string.h>

char* strpbrk(const char* str, const char* reject){
    const char* ptr;
    for (ptr = str; *ptr != '\0'; ptr++) {
        if (strchr(reject, *ptr) != NULL) {
            return (char*)ptr;
        }
    }
    return NULL;
}
