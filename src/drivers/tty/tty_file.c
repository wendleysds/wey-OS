#include <device/tty.h>
#include <kernel/device.h>
#include <device/chrdev.h>
#include <def/errno.h>
#include <fs/stat.h>
#include <fs/vfs.h>

static int tty_file_open(struct inode* ino, struct file* file){
	if(!S_ISCHR(ino->mode) || ino->dev == 0){
		return -EINVAL;
	}

	struct device* dev = device_get_by_devt(ino->dev);
	if(!dev){
		return -ENOENT;
	}

	struct tty_driver* driver = dev->driver_data;
	const int index = MINOR(ino->dev) - driver->minor_start;

	struct tty_struct* tty = tty_ensure_created(driver, index);
	if(IS_ERR(tty)){
		return PTR_ERR(tty);
	}

	file->private_data = tty;
	if(driver->ops->open){
		int res = driver->ops->open(tty, file);
		if(res < 0){
			return res;
		}
	}

	return SUCCESS;
}

static int tty_file_write(struct file* file, const void* buffer, uint32_t count){
	if(!buffer){
		return -EINVAL;
	}

	if(count == 0) return 0;

	struct tty_struct* tty = file->private_data;

	if(!tty->ldisc || !tty->ldisc->ops || !tty->ldisc->ops->write){
		return -ENODEV;
	}

	return tty->ldisc->ops->write(tty, buffer, count);
}

static int tty_file_read(struct file *file, void *buffer, uint32_t count){
	if(!buffer){
		return -EINVAL;
	}

	if(count == 0) return 0;

	struct tty_struct* tty = file->private_data;

	if(!tty->ldisc || !tty->ldisc->ops->read){
		return -ENODEV;
	}

	return tty->ldisc->ops->read(tty, buffer, count);
}

const struct file_operations tty_file_ops = {
	.open = tty_file_open,
	.write = tty_file_write,
	.read = tty_file_read,
};
