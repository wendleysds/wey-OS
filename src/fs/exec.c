#include <wey/binfmts.h>
#include <wey/vfs.h>
#include <wey/mmu.h>
#include <wey/sched.h>
#include <def/config.h>
#include <def/err.h>
#include <lib/string.h>

extern struct binfmt elf_format;
extern struct binfmt script_format;

static struct binfmt* fmts[] = {
	&elf_format, // ELF binary format
	&script_format, // Script binary format
	0x0
};

static struct binprm* alloc_binprm(char* filename, int flags) {
	struct binprm* bprm;
	struct file* file;

	file = vfs_open(filename, flags);
	if (IS_ERR(file)) {
		return ERR_CAST(file);
	}

	bprm = (struct binprm*)kzalloc(sizeof(struct binprm));
	if (!bprm) {
		vfs_close(file);
		return ERR_PTR(NO_MEMORY);
	}

	struct PagingDirectory* dir = mmu_create_page();
	mmu_copy_kernel_to_directory(dir);

	if(IS_ERR_OR_NULL(dir)){
		kfree(bprm);
		vfs_close(file);
		return ERR_PTR(NO_MEMORY);
	}

	struct mm_struct* mm = (struct mm_struct*)kzalloc(sizeof(struct mm_struct));
	if (!mm) {
		kfree(bprm);
		vfs_close(file);
		mmu_destroy_page(dir);
		return ERR_PTR(NO_MEMORY);
	}

	bprm->filename = filename;
	bprm->interp = filename;
	bprm->file = file;

	mm->pageDirectory = dir;
	bprm->mm = mm;

	return bprm;
}

static void bprm_free(struct binprm* bprm) {
	if (!bprm) {
		return;
	}

	if (bprm->file) {
		vfs_close(bprm->file);
	}

	if (bprm->mm) {
		vma_destroy(bprm->mm);
	}

	if (bprm->interp != bprm->filename){
		kfree((void*)bprm->interp);
	}

	kfree(bprm);
}

static int bprm_load(struct binprm* bprm) {
	if (!bprm || !bprm->file) {
		return INVALID_ARG;
	}

	int res = FAILED;
	struct binfmt* fmt;
	for (int i = 0; (fmt = fmts[i]); i++){
		if((res = fmt->load_binary(bprm)) == SUCCESS){
			break;
		}

		if(res == NO_MEMORY){
			return res;
		}
	}

	if(IS_STAT_ERR(res)){
		return INVALID_FILE;
	}

	return SUCCESS;
}

static inline int _count_args_kernel(const char* const* argv) {
	int count = 0;
	while (argv[count]) {
		count++;
		if (count >= PROC_ARG_MAX) {
			return OVERFLOW;
		}
	}
	return count;
}

static int _copy_args_kernel(int count, const char* const* argv, struct binprm* bprm){
	uint8_t* top = (uint8_t*)bprm->curMemTop;
	uint8_t* stack_base = (uint8_t*)((uintptr_t)top - PROC_USER_STACK_SIZE);

	uint8_t* argPtrs[count];
	for (int i = count-1; i >= 0; i--) {
		int len = strlen(argv[i]) + 1;
		if (top - len < stack_base) {
			return OVERFLOW;
		}
		top -= len;
		memcpy(top, argv[i], len);
		argPtrs[i] = top;
	}

	top = (uint8_t*)((uintptr_t)top & ~0xF);

	if (top - sizeof(uint8_t*) < stack_base) {
		return OVERFLOW;
	}
	top -= sizeof(uint8_t*);
	*(uint8_t**)top = NULL;

	for (int i = count-1; i >= 0; i--) {
		if (top - sizeof(uint8_t*) < stack_base) {
			return OVERFLOW;
		}
		top -= sizeof(uint8_t*);
		*(uint8_t**)top = argPtrs[i];
	}

	if (top - sizeof(int) < stack_base) {
		return OVERFLOW;
	}

	top -= sizeof(int);
	*(int*)top = count;

	bprm->curMemTop = (uintptr_t)top; // update

	return SUCCESS;
}

int kernel_exec(const char* pathname, const char* argv[], const char* envp[]) {
	if (!pathname || strlen(pathname) > PATH_MAX || !argv || !envp){
		return INVALID_ARG;
	}

	struct binprm* bprm = alloc_binprm((char*)pathname, 0);
	if(IS_ERR(bprm)){
		return PTR_ERR(bprm);
	}
	
	int res = SUCCESS;

	mmu_page_switch(bprm->mm->pageDirectory);

	res = _count_args_kernel(argv);
	if (IS_STAT_ERR(res)) {
		goto out_fbrpm;
	}

	bprm->argc = res;

	res = _count_args_kernel(envp);
	if (IS_STAT_ERR(res)) {
		goto out_fbrpm;
	}

	bprm->envc = res;

	void* newStack = kzalloc(PROC_USER_STACK_SIZE);
	if(!newStack){
		res = NO_MEMORY;
		goto out_fbrpm;
	}

	void* newKernelStack = kzalloc(PROC_KERNEL_STACK_SIZE);
	if(!newKernelStack){
		res = NO_MEMORY;
		kfree(newStack);
		goto out_fbrpm;
	}

	bprm->curMemTop = ((uintptr_t)newStack + PROC_USER_STACK_SIZE);

	res = _copy_args_kernel(bprm->argc, argv, bprm);
	if (IS_STAT_ERR(res)) {
		goto out_fstack;
	}

	res = _copy_args_kernel(bprm->envc, envp, bprm);
	if (IS_STAT_ERR(res)) {
		goto out_fstack;
	}

	res = mmu_map_pages(
		(void*)PROC_USER_STACK_VIRUTAL_BUTTOM, 
		mmu_translate(newStack), 
		PROC_USER_STACK_SIZE, 
		(FPAGING_P | FPAGING_RW | FPAGING_US)
	);

	if (IS_STAT_ERR(res)) {
		goto out_fstack;
	}

	res = vma_add(bprm->mm, 
		(void*)PROC_USER_STACK_VIRUTAL_BUTTOM, 
		newStack, 
		PROC_USER_STACK_SIZE, 
		(FPAGING_P | FPAGING_RW | FPAGING_US),
		0
	);

	if (IS_STAT_ERR(res)) {
		goto out_fstack;
	}

	res = mmu_map_pages(
		(void*)PROC_KERNEL_STACK_VIRTUAL_BUTTOM, 
		mmu_translate(newKernelStack), 
		PROC_KERNEL_STACK_SIZE, 
		(FPAGING_P | FPAGING_RW)
	);

	if (IS_STAT_ERR(res)) {
		goto out_fstack;
	}

	res = vma_add(bprm->mm,
		(void*)PROC_KERNEL_STACK_VIRTUAL_BUTTOM,
		newKernelStack,
		PROC_KERNEL_STACK_SIZE,
		(FPAGING_P | FPAGING_RW),
		0
	);
	
	if (IS_STAT_ERR(res)) {
		goto out_fstack;
	}
	
	res = bprm_load(bprm);
	if (IS_STAT_ERR(res)) {
		goto out_fstack;
	}

	struct Task* task = pcb_current();
	struct Process* process = task->process;

	if(process->mm){
		vma_destroy(process->mm);
	}

	process->mm = bprm->mm;
	bprm->mm = NULL;

	task->userStack = newStack;
	memset(task->userStack, 0, PROC_USER_STACK_SIZE);

	task->kernelStack = newKernelStack;
	memset(task->kernelStack, 0, PROC_KERNEL_STACK_SIZE);

	// TODO: Implement -> Close all file descriptors

	memset(&task->regs, 0x0, sizeof(struct Registers));

	task->regs.eip = (uint32_t)bprm->entryPoint;

	task->regs.esp = PROC_USER_STACK_VIRUTAL_TOP;
	task->regs.ebp = task->regs.esp;
    task->regs.ss = USER_DATA_SEGMENT;
    task->regs.cs = USER_CODE_SEGMENT;

	goto out_fbrpm;

out_fstack:
	kfree(newStack);
	kfree(newKernelStack);
out_fbrpm:
	bprm_free(bprm);
	return res;
}
