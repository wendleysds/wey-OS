#ifndef _BINARY_FORMATS_H
#define _BINARY_FORMATS_H

#include <lib/mem.h>
#include <stdint.h>

struct binprm{
    const char* filename;
    int argc, envc;
    int fd;

    struct mem_region {
        int size;

        // Where the binary is loaded in memory
        void* loadAddress;

        void* virtualBaseAddress;
        void* virtualEndAddress;

        void* physicalBaseAddress;
        void* physicalEndAddress;
    } mem;

}__attribute__((packed));

struct binfmt{
    int (*load_binary)(struct binprm *bprm);
}__attribute__((packed));

#endif