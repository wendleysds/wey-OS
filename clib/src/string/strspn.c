#include <string.h>

size_t strspn(const char* str, const char* reject) {
    size_t n;
    const char* p;
    for(n=0; *str; str++, n++) {
      for(p=reject; *p && *p != *str; p++);
      if (!*p) break;
    }
    return n;
}
