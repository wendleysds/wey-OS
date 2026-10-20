#include <string.h>

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