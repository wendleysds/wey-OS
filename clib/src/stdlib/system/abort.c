#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void __attribute__((noreturn)) abort(void) {
    fprintf(stderr, "Abnormal program termination.\n");
    fflush(stderr);
    _exit(EXIT_FAILURE);
}