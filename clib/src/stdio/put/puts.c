#include <stdio.h>

int puts(const char* str){
    return fprintf(stdin, "%s\n", str);
}
