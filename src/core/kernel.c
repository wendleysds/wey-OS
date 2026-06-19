#include <kernel/syscall.h>
#include <kernel/interrupt.h>
#include <kernel/sched.h>
#include <kernel/fork.h>
#include <device/terminal.h>
#include <lib/assert.h>
#include <def/err.h>
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
		if(IS_STAT_ERR(res)){
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

static int init(void* args){
	#define TRY(op) do { int __res = (int)(op); BUG_MSG(IS_ERR_VALUE(__res), #op " failed %d", __res); } while(0)

	struct inode* ino;

	// # test 1
	printk("running test 1\n");
	TRY(vfs_mount(0x0, "/", "ramfs", 0x0, 0x0));
	TRY(vfs_mkdir("/home"));
	TRY(vfs_create("/home/file", 0x0));
	TRY(vfs_umount("/"));

	// # test 2
	printk("running test 2\n");
	TRY(vfs_mount(0x0, "/", "ramfs", 0x0, 0x0));
	TRY(vfs_mkdir("/home"));

	TRY(vfs_mount(0x0, "/home", "ramfs", 0x0, 0x0));
	TRY(vfs_create("/home/file", 0x0));

	TRY(vfs_umount("/home"));
	TRY(vfs_umount("/"));

	// # test 3
	printk("running test 3\n");
	TRY(vfs_mount(0x0, "/", "ramfs", 0x0, 0x0));
	TRY(vfs_mkdir("/home"));
	TRY(vfs_mkdir("/home/HD"));
	TRY(vfs_mount(0x0, "/home/HD", "ramfs", 0x0, 0x0));
	
	TRY(vfs_umount("/home/HD"));
	TRY(vfs_umount("/"));

	// # test 4
	printk("running test 4\n");
	TRY(vfs_mount(0x0, "/", "ramfs", 0x0, 0x0));

	TRY(vfs_mkdir("/home"));
	TRY(vfs_mount(0x0, "/home", "ramfs", 0x0, 0x0));

	TRY(vfs_mkdir("/home/user"));
	TRY(vfs_mount(0x0, "/home/user", "ramfs", 0x0, 0x0));

	TRY(vfs_create("/home/user/file", 0x0));

	TRY(vfs_umount("/home/user"));
	TRY(vfs_umount("/home"));
	TRY(vfs_umount("/"));

	// # test 5
	printk("running test 5\n");
	TRY(vfs_mount(0x0, "/", "ramfs", 0x0, 0x0));

	TRY(vfs_mkdir("/mnt"));
	TRY(vfs_mount(0x0, "/mnt", "ramfs", 0x0, 0x0));

	TRY(vfs_mkdir("/mnt/disk"));
	TRY(vfs_mount(0x0, "/mnt/disk", "ramfs", 0x0, 0x0));

	TRY(vfs_mkdir("/mnt/disk/data"));
	TRY(vfs_mount(0x0, "/mnt/disk/data", "ramfs", 0x0, 0x0));

	TRY(vfs_create("/mnt/disk/data/file", 0x0));

	TRY(vfs_umount("/mnt/disk/data"));
	TRY(vfs_umount("/mnt/disk"));
	TRY(vfs_umount("/mnt"));
	TRY(vfs_umount("/"));

	// # test 7
	printk("running test 7\n");
	TRY(vfs_mount(0x0, "/", "ramfs", 0x0, 0x0));

	TRY(vfs_mkdir("/home"));
	TRY(vfs_create("/home/root_file", 0x0));

	TRY(vfs_mount(0x0, "/home", "ramfs", 0x0, 0x0));
	TRY(vfs_create("/home/mounted_file", 0x0));

	TRY(vfs_umount("/home"));

	TRY(ino = vfs_walk("/home/root_file"));
	inode_put(ino);

	TRY(vfs_umount("/"));

	interrupts_disable();

	printk("OK\n");
	while(1) cpu_relax();

	unreachable();
}

static int worker(void* args){
	printk("OK\n");
	while(1){
		cpu_relax();
	}
	return 0;
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
