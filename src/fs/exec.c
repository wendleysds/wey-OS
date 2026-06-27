#include <exec/binfmts.h>
#include <kernel/sched.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <def/err.h>
#include <mm/vma.h>

static LIST_HEAD(formats);

void binfmt_register(struct binfmt* fmt){
	list_add(&fmt->formats, &formats);
}

void binfmt_unregister(struct binfmt* fmt){
	list_remove(&fmt->formats);
}

static inline void _push(void** stack, void* data, unsigned long size) {
	*stack -= size;
	memcpy(*stack, data, size);
}

static inline void* _pop(void** stack, unsigned long size) {
	void* data = *stack;
	*stack += size;
	return data;
}

static int _copy_char_arr(void** stack, const char** arr) {
    if (!arr || !stack) return INVALID_ARG;

    unsigned int count = 0;
    while (arr[count]) {
        if (++count >= PROC_ARG_MAX)
            return OVERFLOW;
    }

    *stack -= sizeof(uintptr_t) * (count + 1);
    uintptr_t* ptr_list = (uintptr_t*)*stack;

    for (unsigned int i = 0; i < count; i++) {
        unsigned int len = strlen(arr[i]) + 1;
        _push(stack, (void*)arr[i], len);
        ptr_list[i] = (uintptr_t)*stack;
    }

    ptr_list[count] = 0;

	// align 16
	*stack = (void*)((uintptr_t)(*stack) & ~0xF);

    return count;
}

static struct binprm* bprm_alloc(const char* filename){
	struct binprm* bprm = kzalloc(sizeof(struct binprm));
	int res = -1;

	if(!bprm){
		return ERR_PTR(NO_MEMORY);
	}

	struct mm_struct* mm = vma_alloc();
	if(!mm){
		kfree(bprm);
		return ERR_PTR(NO_MEMORY);
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
		return ERR_PTR(NO_MEMORY);
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

static int exec_binprm(struct binprm* bprm){
	struct task* cur = current;

	struct vm_region* region = vma_add(
		bprm->mm, 
		PROC_USER_STACK_VIRUTAL_TOP - PROC_USER_STACK_SIZE, 
		PROC_USER_STACK_VIRUTAL_TOP, 
		(MEM_READ | MEM_WRITE | MEM_USER | MEM_GROWSDOWN), 
		PROT_MAP_PRIVATE, 
		NULL, 
		0x0
	);

	if(IS_ERR_VALUE(region)){
		return PTR_ERR(region);
	}

	int res = mmu_context_switch(bprm->mm->ctx);
	if(IS_ERR_VALUE(res)){
		return res;
	}

	if(cur->mm){
		vma_destroy(cur->mm);
		cur->mm = NULL;
	}
	
	cur->mm = bprm->mm;

	for(int i = 0; i < PROC_FD_MAX; i++){
		if(cur->file_table[i]){
			vfs_close(cur->file_table[i]);
			cur->file_table[i] = NULL;
		}
	}

	start_thread_user(
		&cur->regs, 
		bprm->entryPoint, 
		(void*)PROC_USER_STACK_VIRUTAL_TOP
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

	return INVALID_FORMAT;
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

	res = exec_binprm(bprm);
	if(IS_ERR_VALUE(res)){
		bprm_destroy(bprm);
		return res;
	}

	return res;
}
