#include "wey/syscall.h"
#include "wey/vfs.h"
#include <lib/list.h>
#include <lib/font.h>
#include <lib/assert.h>
#include <def/init.h>
#include <def/err.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <mm/kheap.h>
#include <mm/memory.h>
#include <mm/memblock.h>
#include <wey/interrupt.h>
#include <wey/mmu.h>
#include <wey/panic.h>
#include <wey/terminal_struct.h>
#include <wey/terminal.h>
#include <wey/printk.h>
#include <wey/sched.h>
#include <wey/fork.h>

#include <asm/cpu.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

extern void setup_arch();

static inline void module_load(const char* module_name, int (*func)(void)){
	int res;
	if(IS_ERR_VALUE(res = func())){
		panic("\"%s\" module failed with status %d!", module_name, res);
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

static int init(void* args){
	int res = vfs_mount("/dev/hda1", "/", "vfat", 0x0);
	if(res != SUCCESS){
		panic("Failed to mount root! \"%d\"", res);
	}

	return kernel_exec("/init", 0x0, 0x0);
}

static int somw(void* args){
	while(1){
		cpu_relax();
	}
	return 0;
}

__no_return void kmain(){
	memblock_init();

	setup_arch();

	module_load("Memory", memory_init);

	module_load("Terminal", terminal_init);

	module_load("Scheduler", scheduler_init);

	do_initcalls();

	printk("OK\n");

	while(1) cpu_relax();

	interrupts_enable();

	kernel_thread(init, "init", (void*)0xCAFE);
	kernel_thread(somw, "Worker 1", 0x0);
	kernel_thread(somw, "Worker 2", 0x0);
	kernel_thread(somw, "Worker 3", 0x0);

	scheduler_start();

	while(1) cpu_relax();

	__builtin_unreachable();
}

SYSCALL_DEFINE2(tmp_vt_write, const char*, str, int, len){
	for(int i = 0; i < len; i++){
		printk("%c", str[i]);
	}

	return len;
}
