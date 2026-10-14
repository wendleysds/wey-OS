#include <stdio.h>

int putc(int c, FILE* stream){
    if (stream == NULL) return EOF;
    return fputc(c, stream);
}