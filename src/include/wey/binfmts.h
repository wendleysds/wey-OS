#ifndef _BINARY_FORMATS_H
#define _BINARY_FORMATS_H

#include <wey/vfs.h>
#include <wey/vma.h>
#include <stdint.h>

#define BINPRM_BUFF_SIZE 512

struct list_head;

struct binprm{
	int argc, envc;

	void* entryPoint;

	uintptr_t curMemTop;

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
	struct list_head formats; 
};

void binfmt_register(struct binfmt* fmt);
void binfmt_unregister(struct binfmt* fmt);

#endif