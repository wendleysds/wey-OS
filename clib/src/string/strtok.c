#include <string.h>


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