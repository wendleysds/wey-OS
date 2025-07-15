#ifndef _BINARY_FORMATS_H
#define _BINARY_FORMATS_H

#include <stdint.h>

struct binfmt{
    int (*load_binary)(struct binprm *bprm);
}__attribute__((packet));

struct binprm{
    const char* filename;
    const char* fdpath;
    int argc, envc;
    int fd;
}__attribute__((packet));

#endif