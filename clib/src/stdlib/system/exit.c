#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void file_close_all(void);

void __attribute__((noreturn)) _Exit(int status) {
    _exit(status);
}

void __attribute__((noreturn)) exit(int status) {

    // call atexit functions

    fflush(stdout);
    fflush(stderr);

    file_close_all();
    _Exit(status);
}
