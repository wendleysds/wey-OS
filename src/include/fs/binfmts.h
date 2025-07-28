#ifndef _BINARY_FORMATS_H
#define _BINARY_FORMATS_H

#include <core/process.h>
#include <mmu.h>
#include <stdint.h>

struct binprm{
    char* filename;
    int argc, envc;
    int fd;

    void* loadAddress;
    void* entryPoint;

    struct mm_struct* mm;

}__attribute__((packed));

struct binfmt{
    int (*load_binary)(struct binprm *bprm);
}__attribute__((packed));

#endif