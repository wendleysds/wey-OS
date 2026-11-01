#include "kernel/interrupt.h"
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <kernel/sched.h>
#include <exec/binfmts.h>
#include <lib/string.h>
#include <lib/assert.h>
#include <def/errno.h>
#include <mm/vma.h>
#include <mm/page.h>
#include <stdint.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

static LIST_HEAD(formats);

static long strncpy_from_user(char *dst, __user const char *src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        char c;

        if (copy_from_user(&c, src + i, sizeof(c)))
            return -EFAULT;

        dst[i] = c;

        if (c == '\0')
            return (long)i;
    }

    return (long)count;
}

static long strnlen_from_user(__user const char *str, size_t count) {
    for (size_t i = 0; i < count; i++) {
        char c;
        if (copy_from_user(&c, str + i, sizeof(c)))
            return -EFAULT;

        if (c == '\0')
            return (long)i;
    }

    return (long)count;
}

void binfmt_register(struct binfmt* fmt){
	list_add(&fmt->formats, &formats);
}

void binfmt_unregister(struct binfmt* fmt){
	list_remove(&fmt->formats);
}

static struct binprm* bprm_alloc(const char* filename){
	struct binprm* bprm = kzalloc(sizeof(struct binprm));

	if(!bprm){
		return ERR_PTR(-ENOMEM);
	}

	struct mm_struct* mm = vma_alloc();
	if(!mm){
		kfree(bprm);
		return ERR_PTR(-ENOMEM);
	}

	char* kfilename = strdup(filename);
	if(!kfilename){
		vma_destroy(mm);
		kfree(bprm);
		return ERR_PTR(-ENOMEM);
	}

	struct file* file = vfs_open(filename, O_RDONLY, 0x0);
	if(IS_ERR_VALUE(file)){
		vma_destroy(mm);
		kfree(bprm);
		kfree(kfilename);
		return ERR_CAST(file);
	}

	struct paging_ctx* ctx = mmu_create_context();
	if(!ctx){
		vma_destroy(mm);
		vfs_close(file);
		kfree(kfilename);
		kfree(bprm);
		return ERR_PTR(-ENOMEM);
	}

	mm->ctx = ctx;
	bprm->filename = kfilename;
	bprm->mm = mm;
	bprm->file = file;

	return bprm;
}

static void bprm_free(struct binprm* bprm){
	vfs_close(bprm->file);
	if(bprm->interpreter){
		vfs_close(bprm->interpreter);
	}

	if(bprm->filename){
		kfree((void*)bprm->filename);
	}

	kfree(bprm);
}

static void bprm_destroy(struct binprm* bprm){
	if(bprm->mm){
		vma_destroy(bprm->mm);
	}

	BUG_ON(bprm->old_mm);

	bprm_free(bprm);
}

static int binprm_load(struct binprm* bprm){
	struct binfmt* fmt;
	list_for_each_entry(fmt, &formats, formats) {
		if (fmt->load_binary(bprm) == SUCCESS) {
			return SUCCESS;
		}
	}
	return -ENOEXEC;
}

struct user_stack {
	struct mm_struct* mm;
	mem_flags_t mm_flags;
	uintptr_t top;
	uintptr_t sp;
	size_t map_count;
};

static int user_stack_push(
	struct user_stack *stack,
	const void *data,
	size_t size
) {
	const uintptr_t bottom = 
		PROC_USER_STACK_VIRUTAL_TOP - PROC_USER_STACK_INITIAL_SIZE;

	if (size > stack->sp - bottom)
		return -E2BIG;

	const uintptr_t dst = stack->sp - size;

	if(data){
		if (copy_to_user(
			(void *)dst,
			data,
			size
		) != 0) {
			return -EFAULT;
		}
	}else{
		// TODO: implement mem{set,cpy,cmp}_from_user later
		const char zero = 0;
		for(size_t i = 0; i < size; i++){
			(void)copy_to_user(
				(void *)(dst + i),
				&zero,
				sizeof(zero)
			);
		}
	}

	stack->sp = dst;
	return 0;
}

static int user_stack_push_strings(
	struct user_stack *stack,
	const char *const arr[],
	uintptr_t *ptr_arr,
	int count
) {
	if (!stack || !arr || !ptr_arr)
		return -EINVAL;

	for (unsigned int i = count; i > 0; i--) {
		const char *str = arr[i - 1];
		size_t len = strlen(str) + 1;

		int res = user_stack_push(stack, str, len);
		if (res)
			return res;

		ptr_arr[i - 1] = stack->sp;
	}

	ptr_arr[count] = 0;
	return 0;
}

static int binprm_setup_stack(struct binprm* bprm, const char* const argv[], const char* const envp[]){
	struct vm_region *region = vma_add(
		bprm->mm,
		(PROC_USER_STACK_VIRUTAL_TOP - PROC_USER_STACK_INITIAL_SIZE),
		PROC_USER_STACK_VIRUTAL_TOP,
		(MEM_READ | MEM_WRITE | MEM_USER | MEM_GROWSDOWN),
		PROT_MAP_PRIVATE,
		NULL,
		0x0
	);

	if (IS_ERR(region)){
		return PTR_ERR(region);
	}

	struct user_stack user_stack = {
		.mm = bprm->mm,
		.mm_flags = region->mem_flags,
		.top = PROC_USER_STACK_VIRUTAL_TOP,
		.sp = PROC_USER_STACK_VIRUTAL_TOP,
		.map_count = 0
	};

	/* Target build stack
	*  TOP
	* +------------------+
	* | Strings          |
	* +------------------+
	* Padding
	* +------------------+
	* | envp pointers    |
	* +------------------+
	* | argv pointers    |
	* +------------------+
	* | argc             | <- curMemTop
	* +------------------+
	*  BOTTOM
	*/

	int argc = bprm->argc, envc = bprm->envc, res;

	uintptr_t* argv_ptrs = kmalloc(sizeof(uintptr_t) * (argc + 1));
	if(!argv_ptrs) {
		return -ENOMEM;
	}

	uintptr_t* envp_ptrs = kmalloc(sizeof(uintptr_t) * (envc + 1));
	if(!envp_ptrs){
		kfree(argv_ptrs);
		return -ENOMEM;
	}

	res = user_stack_push_strings(&user_stack, argv, argv_ptrs, argc);
	if (res) goto err_args_ptrs;

	res = user_stack_push_strings(&user_stack, envp, envp_ptrs, envc);
	if (res) goto err_args_ptrs;

	const size_t size_args = (argc + envc + 3) * sizeof(uintptr_t);
	const uintptr_t final_sp = ALIGN_DOWN(user_stack.sp - size_args, 16);
	const size_t padding = user_stack.sp - size_args - final_sp;

	res = user_stack_push(&user_stack, NULL, padding);
	if (res) goto err_args_ptrs;

	res = user_stack_push(&user_stack, envp_ptrs, (envc + 1) * sizeof(uintptr_t));
	if (res) goto err_args_ptrs;

	res = user_stack_push(&user_stack, argv_ptrs, (argc + 1) * sizeof(uintptr_t));
	if (res) goto err_args_ptrs;

	res = user_stack_push(&user_stack, &argc, sizeof(uintptr_t));
	if (res) goto err_args_ptrs;

	bprm->curMemTop = user_stack.sp;
	res = 0;

err_args_ptrs:
	kfree(argv_ptrs);
	kfree(envp_ptrs);
	return res;
}

static void binprm_rollback(struct task* cur, struct binprm* bprm){
	if (bprm->old_mm){
		BUG_ON(mmu_context_switch(bprm->old_mm->ctx));
		cur->mm = bprm->old_mm;
		bprm->old_mm = NULL;
	}
}

static int binprm_prepare_exec(struct binprm* bprm, const char* const argv[], const char* const envp[]){
	struct task* cur = current;
	bprm->old_mm = cur->mm;

	cur->mm = bprm->mm;
	int res = mmu_context_switch(cur->mm->ctx);
	if(res < 0){
		cur->mm = bprm->old_mm;
		bprm->old_mm = NULL;
		return res;
	}

	res = binprm_setup_stack(bprm, argv, envp);
	if(res < 0){
		binprm_rollback(cur, bprm);
		return res;
	}

	return SUCCESS;
}

static __no_return void exec_binprm(struct binprm* bprm){
	interrupts_disable();

	struct task* cur = current;

	if(bprm->old_mm){
		vma_destroy(bprm->old_mm);
	}

	start_thread_user(
		&cur->regs, 
		bprm->entryPoint, 
		(void*)bprm->curMemTop
	);

	const char *slash = strrchr(bprm->filename, '/');
	const char *binname = slash ? slash + 1 : bprm->filename;
	
	memset(cur->name, 0x0, sizeof(cur->name));
	strncpy(cur->name, binname, sizeof(cur->name) - 1);

	bprm_free(bprm);
	ret_from_registers(&cur->regs);

	unreachable();
}

static int count_args(const char* const args[]){
	int count = 0;
	while (args[count]) {
		if (++count >= PROC_ARG_MAX) return -1;
	}
	return count;
}

static struct binprm* exec_common(const char* pathname, const char* const argv[], const char* const envp[]){
	struct binprm* bprm = bprm_alloc(pathname);
	if(IS_ERR_VALUE(bprm)){
		return ERR_CAST(bprm);
	}

	int res = binprm_load(bprm);
	if(res < 0){
		bprm_destroy(bprm);
		return ERR_PTR(res);
	}

	int argc = 0, envc = 0;
	if(argv){
		argc = count_args(argv);
		if(argc < 0) {
			bprm_destroy(bprm);
			return ERR_PTR(-E2BIG);
		};
	}

	if(envp){
		envc = count_args(envp);
		if(envc < 0) {
			bprm_destroy(bprm);
			return ERR_PTR(-E2BIG);
		}
	}

	bprm->argc = argc;
	bprm->envc = envc;

	res = binprm_prepare_exec(bprm, argv, envp);
	if(res < 0){
		bprm_destroy(bprm);
		return ERR_PTR(res);
	}
	
	return bprm;
}

int kernel_exec(const char* pathname, const char* const argv[], const char* const envp[]){
	struct binprm* bprm = exec_common(pathname, argv, envp);
	if(IS_ERR(bprm)) {
		return PTR_ERR(bprm);
	};
	exec_binprm(bprm);
}

static void user_args_free(
	char **kargs,
	size_t count
) {
	for (size_t i = 0; i < count; i++) {
		kfree(kargs[i]);
		kargs[i] = NULL;
	}
}

static int copy_args_from_user(
	char **kargs,
	__user const char *const *uargs,
	size_t max_count,
	int *out_count,
	size_t *out_total
) {
	if (!kargs || !uargs || !out_count || !out_total)
		return -EINVAL;

	size_t count = 0;
	size_t total = 0;
	int res = 0;

	*out_count = 0;
	*out_total = 0;

	while (count < max_count) {
		__user const char *ptr;

		if (copy_from_user(&ptr, &uargs[count], sizeof(ptr)) != 0) {
			res = -EFAULT;
			goto rollback;
		}

		if (!ptr)
			goto success;

		const long len = strnlen_from_user(ptr, PROC_ARG_STR_MAX);
		if (len < 0){
			res = -EFAULT;
			goto rollback;
		}

		if ((size_t)len >= PROC_ARG_STR_MAX) {
			res = -E2BIG;
			goto rollback;
		}

		char *str = kmalloc(len + 1);
		if (!str) {
			res = -ENOMEM;
			goto rollback;
		}

		if (copy_from_user(str, ptr, len) != 0) {
			kfree(str);
			res = -EFAULT;
			goto rollback;
		}

		str[len] = '\0';
		kargs[count++] = str;
		total += len + 1;
	}

	res = -E2BIG;
	goto rollback;

success:
	kargs[count] = NULL;
	*out_count = count;
	*out_total = total;
	return 0;

rollback:
	user_args_free(kargs, count);
	return res;
}

SYSCALL_DEFINE3(execve, __user const char*, pathname, __user const char* const*, argv, __user const char* const*, envp){
	char **argv_k = kmalloc(PROC_ARG_MAX * sizeof(char*));
	char **envp_k = kmalloc(PROC_ARG_MAX * sizeof(char*));
	char *kpath = kmalloc(PATH_MAX);

	if (!argv_k || !envp_k || !kpath) {
		kfree(argv_k);
		kfree(envp_k);
		kfree(kpath);
		return -ENOMEM;
	}

	int argc = 0, envc = 0;
	size_t argv_len = 0, envp_len = 0;
	int res;

	res = copy_args_from_user(argv_k, argv, PROC_ARG_MAX, &argc, &argv_len);
	if (res) goto out;

	res = copy_args_from_user(envp_k, envp, PROC_ARG_MAX, &envc, &envp_len);
	if (res) {
		user_args_free(argv_k, argc);
		goto out;
	}

	if(argv_len + envp_len >= PROC_ARG_TOTAL_MAX + 1) {
		res = -E2BIG;
		goto args_free_args;
	}

	long path_len = strncpy_from_user(kpath, pathname, PATH_MAX);
	if (path_len < 0) {
		res = -EFAULT;
		goto args_free_args;
	}

	if (path_len >= PATH_MAX) {
		res = -ENAMETOOLONG;
		goto args_free_args;
	}

	struct binprm* bprm = exec_common(
		kpath,
		(const char* const*)argv_k,
		(const char* const*)envp_k
	);

	if(IS_ERR(bprm)) {
		res = PTR_ERR(bprm);
		goto args_free_args;
	}

args_free_args:
	user_args_free(argv_k, argc);
	user_args_free(envp_k, envc);

out:
	kfree(argv_k);
	kfree(envp_k);
	kfree(kpath);

	if(res < 0){
		return res;
	}

	exec_binprm(bprm);
	unreachable();
}