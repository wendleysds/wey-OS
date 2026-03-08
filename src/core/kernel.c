#include <lib/list.h>
#include <lib/font.h>
#include <def/init.h>
#include <def/err.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <mm/kheap.h>
#include <wey/interrupt.h>
#include <wey/mmu.h>
#include <wey/panic.h>
#include <wey/terminal_struct.h>
#include <wey/terminal.h>
#include <wey/printk.h>
#include <wey/sched.h>
#include <wey/fork.h>

#include <asm/cpu.h>
#include <uapi/headers.h>
#include <stdint.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

extern struct boot_header boot_header;

extern __init void setup_arch();

typedef int (*module_func_t)(void);
static inline void module_load(const char* module_name, module_func_t func){
	int res;
	if(IS_ERR_VALUE(res = func())){
		panic("\"%s\" failed with status %d!", module_name, res);
	}
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

static int test(void* args){
	printk("Args: %#lX\n", args);
	return 0;
}

__no_return void kmain(){
	setup_arch();

	interrupts_disable();

	struct page_metadata* mt = page_init(boot_header.e820_table, boot_header.e820_entries_count);
	if(IS_ERR_VALUE(mt)){
		panic("\"%s\" failed with status %d!", "page allocator", PTR_ERR(mt));
	}

	printk("pages: %d/%d [%d KiB], range 0x%X - 0x%X [%d MiB]\n",
		mt->pages_length, mt->allocated_pages,
		(mt->pages_length / sizeof(struct page)) / KiB(1),
		mt->start_addr, mt->end_addr,
		(mt->end_addr - mt->start_addr) / MiB(1)
	);

	slab_init();

	module_load("MMU", mmu_init);
	module_load("VTerminal", terminal_init);
	module_load("Scheduler", scheduler_init);
	module_load("Fork", fork_init);

	do_initcalls();

	printk("OK\n");

	interrupts_enable();

	pid_t pid = kernel_thread(test, "test", (void*)0xCAFE);
	printk("kernel thread pid: %d\n", pid);

	scheduler_start();

	while(1) cpu_relax();

	__builtin_unreachable();
}
