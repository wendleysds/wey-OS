#include <lib/list.h>
#include <stdint.h>
#include <wey/binfmts.h>
#include <wey/vfs.h>
#include <wey/mmu.h>
#include <wey/sched.h>
#include <def/config.h>
#include <def/err.h>
#include <lib/string.h>

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

static struct binprm* alloc_binprm(char* filename);
static void bprm_free(struct binprm* bprm);

static int exec_binprm();
static int load_binprm();

int kernel_exec(const char* pathname, const char* argv[], const char* envp[]);
