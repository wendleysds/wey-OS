#include <string.h>

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