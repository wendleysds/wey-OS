#include <kernel/init.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/device.h>
#include <def/errno.h>
#include <lib/cpio.h>
#include <fs/vfs.h>
#include <fs/stat.h>

extern char __initramfs_start[];
extern unsigned long __initramfs_size;

extern unsigned long ramdisk_ptr;
extern unsigned long ramdisk_size;

static int unpack(const uint8_t* initrd_ptr, size_t initrd_psize){
	int last_err = 0;

	const uint8_t* cursor = NULL;
	struct cpio_file_iter cpio;
	while((last_err = cpio_initramfs_iterate(
		initrd_ptr,
		initrd_psize,
		&cursor,
		&cpio)) > 0)
	{
		if(!cpio.filesize) continue;

		printk("Initrd: name=%s mode=%#o filesize=%lu\n",
			cpio.name ? cpio.name : "(null)",
			cpio.mode,
			(unsigned long)cpio.filesize
		);

		if(S_ISDIR(cpio.mode)){
			if((last_err = vfs_mkdir(cpio.name))){
				printk("Initrd: mkdir failed: %d\n", last_err);
				return last_err;
			}

			continue;
		}

		if(S_ISREG(cpio.mode)){
			struct file* file = vfs_open(
				cpio.name,
				O_CREAT | O_TRUNC,
				cpio.mode
			);

			if(IS_ERR(file)){
				printk("Initrd: open failed: %ld\n", PTR_ERR(file));
				return PTR_ERR(file);
			}

			int written = vfs_write(file, cpio.content_ptr, cpio.filesize);
			if(written < 0){
				printk("Initrd: write failed: %d\n", written);
				return written;
			}

			if((size_t)written != cpio.filesize){
				return -EIO;
			}

			vfs_close(file);
		}else{
			if((last_err = vfs_mknod(
				cpio.name,
				cpio.mode,
				MKDEV(cpio.devmajor, cpio.devminor)
			))){
				printk("Initrd: mknod failed: %d\n", last_err);
				return last_err;
			}
		}
	}

	return last_err;
}

static int unpack_initrd(void){
	int res = OK;

	if(__initramfs_size || ramdisk_size){
		printk("Unpacking initrd ...\n");
	}

	if((size_t)__initramfs_size != 0){
		res = unpack(
			(void*)__initramfs_start,
			(size_t)__initramfs_size
		);

		if(res) panic("Initrd: internal ramfs unpack failed! %d", res);
	}

	if(ramdisk_size != 0){
		res = unpack(
			(void*)ramdisk_ptr,
			(size_t)ramdisk_size
		);
	}

	return res;
}

late_initcall(unpack_initrd);
