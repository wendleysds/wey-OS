#include <kernel/init.h>
#include <kernel/panic.h>
#include <fs/vfs.h>

static __init int init_rootfs(){
	if(vfs_mount(NULL, "/", "ramfs", 0x0, 0x0)){
		panic("Failed to mount root!");
	}

	return 0;
}

rootfs_initcall(init_rootfs);
