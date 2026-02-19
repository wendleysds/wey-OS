#include "def/compile.h"
#include "def/config.h"
#include "def/err.h"
#include "def/status.h"
#include "wey/sched/task.h"
#include <def/init.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <stdint.h>
#include <wey/interrupt.h>
#include <wey/mmu.h>
#include <wey/panic.h>

#include <lib/font.h>
#include <wey/terminal_struct.h>
#include <wey/terminal.h>
#include <wey/printk.h>
#include <mm/kheap.h>

#include <wey/timer.h>
#include <wey/sched.h>

#include <uapi/headers.h>
#include <lib/string.h>

#define halt() do {__asm__ volatile("hlt");}while(1)

extern struct boot_header boot_header;

extern __init void setup_arch();

static inline __no_return void module_failed(const char* module, int res){
	panic("%s failed with status %d!", module, res);
	halt();
}

extern void start_thread(struct registers* regs, void* entry_point, void* user_stack);
extern asmlinkage __no_return void _ret_from_fork(void);

static void _routine_test(){
	printk("Hello, World!");
	halt();
}

static int _kernel_thread(struct task* task){
	const int stack_size = 1024;
	char* ksp = kmalloc(stack_size);
	char* usp = kmalloc(stack_size);

	if(!ksp || !usp){
		return NO_MEMORY;
	}

	ksp += stack_size;

	start_thread(&task->regs, _routine_test, usp + stack_size);

	task->regs.cs = GDT_KERNEL_CODE;
	task->regs.ss = GDT_KERNEL_DATA;

	#define push(obj, size) do { \
		ksp = (char*)ksp - size; \
		memcpy(ksp, obj, size); \
	} while(0)

	int returns = (int)_ret_from_fork;

	push(&task->regs, sizeof(task->regs));
	push(&returns, sizeof(int));

	task->regs.ksp = (unsigned long)ksp;
	task->regs.ax = 0xCAFE;

	printk("%X\n", (uint32_t*)ksp);

	#undef push

	return SUCCESS;
}

__no_return void kmain(){
	int res;

	setup_arch();

	interrupts_disable();

	res = PTR_ERR(
		page_init(boot_header.e820_table, boot_header.e820_entries_count)
	);

	if(IS_ERR_VALUE(res)){
		module_failed("page allocator", res);
	}

	slab_init();

	res = mmu_init();
	if(IS_ERR_VALUE(res)){
		module_failed("MMU", res);
	}

	res = terminal_init();
	if(IS_ERR_VALUE(res)){
		module_failed("VTerminal", res);
	}

	res = timer_init();
	if(IS_ERR_VALUE(res)){
		module_failed("Timer", res);
	}

	res = scheduler_init();
	if(IS_ERR_VALUE(res)){
		module_failed("Scheduler", res);
	}

	struct task* t1 = task_create("task 1", 0);
	if(!t1){
		panic("NO MEMORY");
	}

	res = _kernel_thread(t1);
	if(IS_ERR_VALUE(res)){
		module_failed("KThrd", res);
	}

	scheduler_add(t1);

	printk("OK");

	interrupts_enable(); // Start scheduler

	halt();

	__builtin_unreachable();
}
