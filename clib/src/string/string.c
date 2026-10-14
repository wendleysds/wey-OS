#include <string.h>

size_t strlen(const char *s) {
    const char *p = s;

    while(*p) p++;
    return p - s;
}

size_t strnlen(const char* s, size_t max) {
    size_t len = 0;
    while(len < max && *s) {
        len++;
        s++;
    }
    return len;
}

char* strcpy(char* dest, const char* src){
    char* d = dest;

    while((*d++ = *src++));

    return dest;
}

char* strncpy(char* dest, const char* src, size_t n){
    char* d = dest;

    while(n-- && (*d++ = *src++));
    while(n--) *d++ = '\0';

    return dest;
}

char* strcat(char* dest, const char* src){
    char* d = dest;

    while(*d) d++;
    while((*d++ = *src++));

    return dest;
}

char* strncat(char* dest, const char* src, size_t n){
    char* d = dest;

    while(*d) d++;
    while(n-- && (*d++ = *src++));

    return dest;
}

int strcmp(const char* a, const char* b){
    while(*a && *b){
        if(*a != *b){
            return *a - *b;
        }
        a++;
        b++;
    }

    return *a - *b;
}

int strncmp(const char* a, const char* b, size_t n){
    while(n-- && *a && *b){
        if(*a != *b){
            return *a - *b;
        }
        a++;
        b++;
    }

    return *a - *b;
}

char* memchr(const void* ptr, int c, size_t n){
    const unsigned char* p = (const unsigned char*)ptr;

    while(n--){
        if(*p == c){
            return (char*)p;
        }
        p++;
    }

    return NULL;
}

char* strchr(const char* s, int c){
    while(*s){
        if(*s == c){
            return (char*)s;
        }
        s++;
    }

    return NULL;
}

char* strrchr(const char* s, int c){
    char* found = NULL;

    while(*s){
        if(*s == c){
            found = (char*)s;
        }
        s++;
    }

    return found;
}

char* strstr(const char* haystack, const char* needle){
    const char* h = haystack;
    const char* n = needle;

    while(*h){
        if(*h == *n){
            const char* hp = h;
            const char* np = n;

            while(*np && *hp && *hp == *np){
                hp++;
                np++;
            }

            if(!*np){
                return (char*)h;
            }
        }
        h++;
    }

    return NULL;
}

char* strtok(char* s, const char* delimiters){
    static char* last;
    char* token;

    if(s == NULL){
        s = last;
    }

    while(*s && strchr(delimiters, *s)){
        s++;
    }

    if(!*s){
        last = NULL;
        return NULL;
    }

    token = s;
    while(*s && !strchr(delimiters, *s)){
        s++;
    }

    if(*s){
        *s = '\0';
        last = s + 1;
    }else{
        last = NULL;
    }

    return token;
}

size_t strxfrm(char* dest, const char* src, size_t n){
    size_t src_len = strlen(src);
    if (n == 0) {
        return src_len;
    }

    if (dest != NULL) {
        size_t copy_len = (src_len < n) ? src_len : (n - 1);
        
        memcpy(dest, src, copy_len);
        dest[copy_len] = '\0';
    }

    return src_len;
}