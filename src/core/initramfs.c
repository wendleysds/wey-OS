#include <kernel/init.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <def/err.h>
#include <lib/cpio.h>
#include <fs/vfs.h>
#include <fs/stat.h>

extern char __initramfs_start[];
extern unsigned long __initramfs_size;

int unpack_initrd(){
	if((size_t)__initramfs_size == 0){
		return OK;
	}

	int last_err = 0;

	const uint8_t* cursor = NULL;
	struct cpio_file_iter cpio;
	while((last_err = cpio_initramfs_iterate(
		__initramfs_start,
		(size_t)__initramfs_size,
		&cursor,
		&cpio)) == OK)
	{
		printk("initrd: name=%s mode=%#o filesize=%lu content=%p\n",
			cpio.name ? cpio.name : "(null)",
			cpio.mode,
			(unsigned long)cpio.filesize,
			cpio.content_ptr);

		if(S_ISDIR(cpio.mode)){
			if((last_err = vfs_mkdir(cpio.name))){
				printk("initrd: mkdir failed: %d\n", last_err);
				return last_err;
			}
		}

		else if(S_ISREG(cpio.mode)){
			struct file* file = vfs_open(cpio.name, O_CREAT | O_TRUNC, cpio.mode);
			if(IS_ERR(file)){
				printk("initrd: open failed: %ld\n", PTR_ERR(file));
				return PTR_ERR(file);
			}

			int written = vfs_write(file, cpio.content_ptr, cpio.filesize);
			if(written < 0){
				printk("initrd: write failed: %d\n", written);
				return written;
			}

			if((size_t)written != cpio.filesize){
				return ERROR_IO;
			}

			vfs_close(file);
		}
	}

	return last_err == END_OF_FILE ? OK : last_err;
}
