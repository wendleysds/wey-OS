#include "kernel/printk.h"
#include <kernel/syscall.h>
#include <kernel/interrupt.h>
#include <kernel/sched.h>
#include <kernel/fork.h>
#include <device/terminal.h>
#include <lib/assert.h>
#include <def/errno.h>
#include <fs/vfs.h>
#include <mm/memory.h>
#include <mm/memblock.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

extern void setup_arch();

static inline void module_load(const char* module_name, int (*func)(void)){
	int res = func();

	if(IS_ERR_VALUE(res)){
		panic("\"%s\" module failed with status %d!", module_name, res);
	}

	printk("Module \"%s\" OK\n", module_name);
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
	for(initcall_entry_t* fn = initcall_levels[level]; fn < initcall_levels[level+1]; fn++){
		int res = initcall_from_entry(fn)();
		if(IS_ERR_VALUE(res)){
			panic("initcall '%#x' returned status %d\n", 
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

#include <fs/stat.h>
#include <kernel/device.h>

static int init(void* args){
	//interrupts_disable();

	//kernel_exec("/init", 0x0, 0x0);

	vfs_mknod("/tty0", 00755 | S_IFCHR, MKDEV(4, 0));
	struct file* tty = vfs_open("/tty0", 0x0, 0x0);

	while(1){
		char buffer[64];
		int readed = vfs_read(tty, buffer, sizeof(buffer));
		buffer[readed] = '\0';
		printk("readed %d: %s\n", readed, buffer);
	}

	while(1) cpu_relax();
	unreachable();
}

__no_return void kmain(){
	memblock_init();

	setup_arch();

	terminal_early_init();

	module_load("Memory", memory_init);

	module_load("Terminal", terminal_init);

	module_load("Scheduler", scheduler_init);

	do_initcalls();

	interrupts_enable();

	kernel_thread(init, "init", (void*)0xCAFE);

	scheduler_start();

	while(1) cpu_relax();

	unreachable();
}

SYSCALL_DEFINE2(tmp_vt_write, const char*, str, int, len){
	printk("%.*s", len, str);
	return len;
}
