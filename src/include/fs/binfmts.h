#ifndef _BINARY_FORMATS_H
#define _BINARY_FORMATS_H

#include <core/process.h>
#include <fs/vfs.h>
#include <mmu.h>
#include <stdint.h>

#define BINPRM_BUFF_SIZE 256

struct binprm{
    int argc, envc;

    void* entryPoint;

	uintptr_t curMemTop;

	struct file* executable;
	struct file* interpreter;
    struct file* file;

	const char* filename;
	const char* interp;
	const char* fdpath;

    int execfd;

    struct mm_struct* mm;

	char buff[BINPRM_BUFF_SIZE];
}__attribute__((packed));

struct binfmt{
    int (*load_binary)(struct binprm *bprm);
}__attribute__((packed));

#endif