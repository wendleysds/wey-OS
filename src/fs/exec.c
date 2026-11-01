#include <exec/binfmts.h>
#include <kernel/uaccess.h>
#include <kernel/sched.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <def/errno.h>
#include <mm/vma.h>

static LIST_HEAD(formats);

void binfmt_register(struct binfmt* fmt){
	list_add(&fmt->formats, &formats);
}

void binfmt_unregister(struct binfmt* fmt){
	list_remove(&fmt->formats);
}

struct user_stack {
	__user uintptr_t sp;
};

static int user_stack_align(
	struct user_stack *stack,
	size_t alignment
) {
	uintptr_t aligned = stack->sp & ~(alignment - 1);
	size_t padding = stack->sp - aligned;

	if (padding) {
		unsigned char zero[16] = {0};

		if (padding > sizeof(zero))
			return -EINVAL;

		if (copy_to_user(
			(void *)aligned,
			zero,
			padding
		) != 0) {
			return -EFAULT;
		}

		stack->sp = aligned;
	}

	return 0;
}


static int user_stack_push(
	struct user_stack *stack,
	const void *data,
	size_t size
) {
	uintptr_t dst = stack->sp - size;

	if (dst < PROC_USER_STACK_VIRUTAL_TOP -
			PROC_USER_STACK_INITIAL_SIZE)
		return -E2BIG;

	if (copy_to_user(
		(void *)dst,
		data,
		size
	) != 0) {
		return -EFAULT;
	}

	stack->sp = dst;

	return 0;
}

static int user_stack_push_strings(
	struct user_stack *stack,
	const char *const arr[],
	uintptr_t *user_ptrs,
	int *out_count
) {
	if (!stack || !arr || !user_ptrs || !out_count)
		return -EINVAL;

	unsigned int count = 0;

	while (arr[count]) {
		if (++count >= PROC_ARG_MAX)
			return -E2BIG;
	}

	/*
	* Copy strings in reverse order.
	* Keep their userspace addresses in kernel memory.
	*/
	for (unsigned int i = count; i > 0; i--) {
		const char *str = arr[i - 1];
		size_t len = strlen(str) + 1;

		int res = user_stack_push(stack, str, len);

		if (res)
			return res;

		user_ptrs[i - 1] = stack->sp;
	}

	user_ptrs[count] = 0;
	*out_count = count;

	return 0;
}

static int bprm_setup_stack(
	struct binprm *bprm,
	const char *const argv[],
	const char *const envp[],
	uintptr_t *user_sp
) {
	(void)bprm;

	struct user_stack stack = {
		.sp = PROC_USER_STACK_VIRUTAL_TOP
	};

	uintptr_t argv_ptrs[PROC_ARG_MAX + 1];
	uintptr_t envp_ptrs[PROC_ARG_MAX + 1];

	int argc = 0;
	int envc = 0;
	int res;

	/*
	* Copy argument strings.
	*/
	res = user_stack_push_strings(
		&stack,
		argv,
		argv_ptrs,
		&argc
	);

	if (res)
		return res;

	/*
	* Copy environment strings.
	*/
	res = user_stack_push_strings(
		&stack,
		envp,
		envp_ptrs,
		&envc
	);

	if (res)
		return res;

	/*
	* Build final stack:
	*
	* [LOWER ADDRESSES]
	*
	* argc
	* argv[0]
	* ...
	* argv[argc] = NULL
	* envp[0]
	* ...
	* envp[envc] = NULL
	*
	* [HIGHER ADDRESSES]
	*/

	uintptr_t final_stack[
		1 + (PROC_ARG_MAX + 1) * 2
	];

	final_stack[0] = argc;

	memcpy(
		&final_stack[1],
		argv_ptrs,
		(argc + 1) * sizeof(uintptr_t)
	);

	memcpy(
		&final_stack[argc + 2],
		envp_ptrs,
		(envc + 1) * sizeof(uintptr_t)
	);

	size_t size = (argc + envc + 3) * sizeof(uintptr_t);

	/*
	* Reserve space and align the initial ESP.
	*/
	uintptr_t bottom = PROC_USER_STACK_VIRUTAL_TOP -
					PROC_USER_STACK_INITIAL_SIZE;

	if (size > stack.sp - bottom)
		return -E2BIG;

	uintptr_t final_sp = (stack.sp - size) & ~(uintptr_t)0xF;

	if (final_sp < bottom)
		return -E2BIG;

	/*
	* Only one copy for the final pointer table.
	*/
	if (copy_to_user(
		(void *)final_sp,
		final_stack,
		size
	) != 0) {
		return -EFAULT;
	}

	*user_sp = final_sp;

	return 0;
}

static struct binprm* bprm_alloc(const char* filename){
	struct binprm* bprm = kzalloc(sizeof(struct binprm));
	int res = -1;

	if(!bprm){
		return ERR_PTR(-ENOMEM);
	}

	struct mm_struct* mm = vma_alloc();
	if(!mm){
		kfree(bprm);
		return ERR_PTR(-ENOMEM);
	}

	struct file* file = vfs_open(filename, O_RDONLY, 0x0);
	if(IS_ERR_VALUE(file)){
		res = PTR_ERR(file);
		goto out_free;
	}

	struct paging_ctx* ctx = mmu_create_context();
	if(!ctx){
		vma_destroy(mm);
		vfs_close(file);
		return ERR_PTR(-ENOMEM);
	}

	mm->ctx = ctx;
	bprm->filename = filename;
	bprm->mm = mm;
	bprm->file = file;

	return bprm;

out_free:
	vma_destroy(mm);	
	kfree(bprm);
	return ERR_PTR(res);
}

static void bprm_destroy(struct binprm* bprm){
	if(bprm->mm){
		vma_destroy(bprm->mm);
		bprm->mm = NULL;
	}

	vfs_close(bprm->file);
	if(bprm->interpreter){
		vfs_close(bprm->interpreter);
	}

	kfree(bprm);
}

static inline void bprm_free(struct binprm* bprm){
	kfree(bprm);
}

static int exec_binprm(struct binprm* bprm, const char* argv[], const char* envp[]){
	struct task* cur = current;
	int res;

	struct vm_region* region = vma_add(
		bprm->mm, 
		(PROC_USER_STACK_VIRUTAL_TOP - PROC_USER_STACK_INITIAL_SIZE), 
		PROC_USER_STACK_VIRUTAL_TOP, 
		(MEM_READ | MEM_WRITE | MEM_USER | MEM_GROWSDOWN), 
		PROT_MAP_PRIVATE, 
		NULL, 
		0x0
	);

	if(IS_ERR_VALUE(region)){
		return PTR_ERR(region);
	}

	res = mmu_context_switch(bprm->mm->ctx);
	if(IS_ERR_VALUE(res)){
		return res;
	}

	struct mm_struct *old_mm = cur->mm;
	cur->mm = bprm->mm;

	uintptr_t user_stack;
	res = bprm_setup_stack(
		bprm,
		argv,
		envp,
		&user_stack
	);

	if (res){
		BUG_ON(!mmu_context_switch(old_mm->ctx));
		cur->mm = old_mm;
		return res;
	}

	if(old_mm){
		vma_destroy(old_mm);
	}

	start_thread_user(
		&cur->regs, 
		bprm->entryPoint, 
		(void*)user_stack
	);

	char* binname = strrchr(bprm->filename, '/') + 1;
	memset(cur->name, 0x0, sizeof(cur->name));
	strncpy(cur->name, binname, sizeof(cur->name));

	bprm_free(bprm);
	ret_from_registers(&cur->regs);

	unreachable();
}

static int load_binprm(struct binprm* bprm){
	struct binfmt* fmt;
	list_for_each_entry(fmt, &formats, formats){
		if(fmt->load_binary(bprm) == SUCCESS){
			return SUCCESS;
		}
	}

	return -EINVAL;
}

int kernel_exec(const char* pathname, const char* argv[], const char* envp[]){
	struct binprm* bprm = bprm_alloc(pathname);
	if(IS_ERR_VALUE(bprm)){
		return PTR_ERR(bprm);
	}

	int res = load_binprm(bprm);
	if(IS_ERR_VALUE(res)){
		bprm_destroy(bprm);
		return res;
	}

	res = exec_binprm(bprm, argv, envp);
	if(IS_ERR_VALUE(res)){
		bprm_destroy(bprm);
		return res;
	}

	return res;
}
