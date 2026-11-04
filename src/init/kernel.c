#include <kernel/syscall.h>
#include <kernel/interrupt.h>
#include <kernel/kallsyms.h>
#include <kernel/device.h>
#include <kernel/sched.h>
#include <kernel/clock.h>
#include <kernel/fork.h>
#include <device/terminal.h>
#include <lib/assert.h>
#include <def/errno.h>
#include <def/linker.h>
#include <fs/vfs.h>
#include <fs/stat.h>
#include <fs/fdtable.h>
#include <fs/dcache.h>
#include <mm/memory.h>
#include <mm/memblock.h>
#include <mm/page.h>

#include <asm/page.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

extern void setup_arch();

extern struct task* init_task;
extern struct mount *root_mount;

static const char *argv_init[] = { NULL, NULL };
static const char *envp_init[] = { "HOME=/", NULL };

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
		initcall_t initcall = initcall_from_entry(fn);

		int res = initcall();
		if(IS_ERR_VALUE(res)){
			const char* name = NULL;
			kallsyms_lookup_symbol(
				(uintptr_t)initcall, 
				&name, 
				NULL
			);

			panic("initcall<%d>(%s): failed \"%d\"\n", 
				level,
				name ? name : "???",
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

static void print_args(const char** args){
	for (const char *const *p = args; *p; p++){
		printk("Init:    [%d]=\"%s\"\n", p - args, *p);
	}
}

static int run_init_process(const char *pathname) {
	argv_init[0] = pathname;
	printk("Init: Run %s as init process\n", pathname);
	printk("Init: - with arguments:\n");
	print_args(argv_init);

	printk("Init: - with environment:\n");
	print_args(envp_init);

	return kernel_exec(pathname, argv_init, envp_init);
}

static void try_to_run_init_process(const char *pathname) {
	int ret = run_init_process(pathname);

	if (ret && ret != -ENOENT) {
		printk("init(%s): failed to execute!(%d)\n", pathname, ret);
	}
}

static __no_return void init(void){
	buddy_add_memory(
		__va(EARLY_STACK_BOTTOM),
		EARLY_STACK_SIZE 
	);

	buddy_add_memory(
		(uintptr_t)(__init_begin),
		(size_t)(__init_end - __init_begin)
	);

	try_to_run_init_process("/init");
	try_to_run_init_process("/bin/init");
	try_to_run_init_process("/bin/sh");

	unreachable();
}

static __init int rest_init(void* unused){
	init_task = current;

	do_initcalls();

	int res = vfs_mknod("/tty", 00755 | S_IFCHR, MKDEV(4, 0));
	if(res) return res;

	struct file* tty = vfs_open("/tty", O_RDWR, 00);
	if(IS_ERR(tty)){
		return PTR_ERR(tty);
	}

	current->cwd.dentry = dentry_get(root_mount->mnt_root);
	current->cwd.mount = root_mount;

	res = task_add_file(current, tty); // stdin   0
	if(res < 0) return res;

	res = task_add_file(current, tty); // stdout  1
	if(res < 0) return res;

	res = task_add_file(current, tty); // stderr  2
	if(res < 0) return res;

	file_put(tty); // file_table now holds a reference, so we can release ours

	init();

	unreachable();
}

__no_return __init void kmain(){
	memblock_init();

	setup_arch();

	terminal_early_init();

	module_load("Memory", memory_init);

	module_load("Clock", clock_init);

	module_load("Interrupt", interrupt_init);

	module_load("Terminal", terminal_init);

	module_load("Scheduler", scheduler_init);

	pid_init();

	kernel_thread(rest_init, "init", NULL);

	scheduler_start();

	interrupts_enable();

	while(1) cpu_relax();
}

SYSCALL_DEFINE2(tmp_vt_write, const char*, str, int, len){
	printk("%.*s", len, str);
	return len;
}
