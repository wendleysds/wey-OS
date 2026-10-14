#include <stdio.h>

int fputs(const char* str, FILE* stream){
    if (!stream) return EOF;
    return fprintf(stream, "%s", str);
}
