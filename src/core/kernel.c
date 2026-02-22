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

#include <wey/sched.h>

#include <uapi/headers.h>
#include <lib/string.h>

#define halt() do {__asm__ volatile("hlt");}while(1)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

extern struct boot_header boot_header;

extern __init void setup_arch();

static inline __no_return void module_failed(const char* module, int res){
	panic("%s failed with status %x!", module, res);
	halt();
}

extern void start_thread(struct registers* regs, void* entry_point, void* user_stack);
extern asmlinkage __no_return void ret_from_fork(void);

static void _routine_test(){
	printk("Hello, World!\n");
	halt();
}

static int _kernel_thread(struct task* task){
	const int stack_size = 4096;
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

	int returns = (int)ret_from_fork;

	push(&task->regs, sizeof(task->regs));
	push(&returns, sizeof(int));

	task->regs.ksp = (unsigned long)ksp;
	task->regs.ax = 0xCAFE;

	#undef push

	return SUCCESS;
}

static initcall_entry_t* initcall_levels[] __initdata = {
	__initcall0_start,
	__initcall1_start,
	__initcall2_start,
	__initcall3_start,
	__initcall4_start,
	__initcall5_start,
	__initcall6_start,
	__initcall7_start,
	__initcall_end,
};

static __init void _do_initcall_level(int level){
	initcall_entry_t *fn;
	
	for(fn = initcall_levels[level]; fn < initcall_levels[level+1]; fn++){
		int res = initcall_from_entry(fn)();
		if(IS_STAT_ERR(res)){
			panic("initcall '0x%X' returned status %d\n", 
				initcall_from_entry(fn), 
				res
			);
		}
	}
}

static __init void do_initcalls(){
	for(int i = 0; i < ARRAY_SIZE(initcall_levels) - 1; i++){
		_do_initcall_level(i);
	}
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

	struct page_metadata* mt = (struct page_metadata*)res;
	printk("pages: %d/%d [%d KiB], range 0x%X - 0x%X [%d MiB]\n",
		mt->pages_length, mt->allocated_pages,
		(mt->pages_length / sizeof(struct page)) / KiB(1),
		mt->start_addr, mt->end_addr,
		(mt->end_addr - mt->start_addr) / MiB(1)
	);

	slab_init();

	res = mmu_init();
	if(IS_ERR_VALUE(res)){
		module_failed("MMU", res);
	}

	res = terminal_init();
	if(IS_ERR_VALUE(res)){
		module_failed("VTerminal", res);
	}

	res = scheduler_init();
	if(IS_ERR_VALUE(res)){
		module_failed("Scheduler", res);
	}

	do_initcalls();

	struct task* t1 = task_create("task 1", 0);
	if(!t1){
		panic("NO MEMORY");
	}

	res = _kernel_thread(t1);
	if(IS_ERR_VALUE(res)){
		module_failed("KThrd", res);
	}

	struct task* t2 = task_create("task 2", 0);
	if(!t2){
		panic("NO MEMORY");
	}

	res = _kernel_thread(t2);
	if(IS_ERR_VALUE(res)){
		module_failed("KThrd", res);
	}

	scheduler_add(t1);
	scheduler_add(t2);

	printk("OK\n");


	interrupts_enable(); // Start scheduler

	halt();

	__builtin_unreachable();
}
