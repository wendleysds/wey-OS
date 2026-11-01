#ifndef _BINARY_FORMATS_H
#define _BINARY_FORMATS_H

#include <lib/list.h>
#include <stdint.h>

#define BINPRM_BUFF_SIZE 512

struct file;

struct binprm{
	int argc, envc;

	struct mm_struct* mm;
	struct mm_struct *old_mm;

	void* entryPoint;

	uintptr_t curMemTop;

	struct file* interpreter;
	struct file* file;

	const char* filename;

	char buff[BINPRM_BUFF_SIZE];
}__attribute__((packed));

struct binfmt{
    int (*load_binary)(struct binprm *bprm);
	struct list_head formats; 
};

void binfmt_register(struct binfmt* fmt);
void binfmt_unregister(struct binfmt* fmt);

#endif