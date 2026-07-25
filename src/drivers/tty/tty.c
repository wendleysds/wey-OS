#include <device/tty.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <kernel/device.h>
#include <device/chrdev.h>
#include <mm/kheap.h>
#include <def/errno.h>
#include <lib/string.h>
#include <lib/stdio.h>
#include <fs/vfs.h>
#include <fs/stat.h>

#define TTY_MAX_NAME 32

extern const struct tty_ldisc_ops tty_ldisc_ops;

static LIST_HEAD(drivers);

struct tty_struct* tty_ensure_created(struct tty_driver* driver, const int index){
	struct tty_struct* tty = driver->ttys[index];
	if(!tty){
		tty = kmalloc(sizeof(struct tty_struct));
		if(!tty){
			return ERR_PTR(-ENOMEM);
		}

		tty->driver = driver;
		tty->index = index;
		spinlock_init(&tty->lock);

		int res = driver->ops->install(driver, tty);
		if(res < 0){
			kfree(tty);
			return ERR_PTR(res);
		}

		res = tty_ldisc_ops.open(tty);
		if(res < 0){
			kfree(tty);
			return ERR_PTR(res);
		}

		driver->ttys[index] = tty;
		tty->ops = tty->driver->ops;
	}

	return tty;
}

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
	spin_lock(&tty->lock);

	if(!tty->ops->write){
		spin_unlock(&tty->lock);
		return -ENOSYS;
	}

	int res = tty->ops->write(tty, buffer, count);

	spin_unlock(&tty->lock);
	return res;
}

static const struct file_operations tty_file_ops = {
	.open = tty_file_open,
	.write = tty_file_write
};

struct tty_driver* tty_alloc_drive(){
	struct tty_driver* driver = kzalloc(sizeof(struct tty_driver));
	if(driver){
		INIT_LIST_HEAD(&driver->list);
	}
	return driver;
}

int tty_register_driver(struct tty_driver* driver){
	if(!driver){
		return -EINVAL;
	}

	size_t name_len = strnlen(driver->name, TTY_MAX_NAME);
	if(name_len == 0){
		return -EINVAL;
	}

	if(name_len + 1 >= TTY_MAX_NAME){
		return -ENAMETOOLONG;
	}

	dev_t devt = MKDEV(driver->major, driver->minor_start);

	if(devt == 0){
		return -EINVAL;
	}

	struct device** devs = kcalloc(sizeof(struct device*), driver->num);
	if(!devs){
		return -ENOMEM;
	}

	struct tty_struct** ttys = kcalloc(sizeof(struct tty_struct*), driver->num);
	if(!ttys){
		kfree(devs);
		return -ENOMEM;
	}

	int res = chardev_register(
		driver->major,
		driver->minor_start,
		driver->num,
		driver->name,
		&tty_file_ops
	);

	if(res < 0){
		kfree(ttys);
		kfree(devs);
		return res;	
	}

	if(driver->major == 0){
		driver->major = res;
	}

	size_t writed;
	char buffer[TTY_MAX_NAME];
	for(unsigned int i = 0; i < driver->num; i++){
		devt = MKDEV(driver->major, driver->minor_start + i);

		writed = snprintf(buffer, TTY_MAX_NAME, "%s%d", driver->name, i);
		if(writed >= TTY_MAX_NAME){
			printk("TTY: Name for tty \"%s%d\" too long!", driver->name, i);
			res = -ENAMETOOLONG;
			goto free_all;
		}

		buffer[writed] = '\0';

		struct device* dev = device_create(devt, driver, buffer);
		if(IS_ERR(dev)) {
			res = PTR_ERR(dev);
			goto free_all;
		}

		devs[i] = dev;
	}

	driver->major = res;
	driver->devs = devs;
	driver->ttys = ttys;

	INIT_LIST_HEAD(&driver->list);
	list_add(&driver->list, &drivers);

	return driver->major;

free_all:
	for(unsigned int i = 0; i < driver->num; i++){
		if(devs[i]){
			device_unregister(devs[i]);
		     	kfree((void*)devs[i]->name);
			kfree(devs[i]);
		}else{
			break;
		}
	}

	chardev_unregister(driver->major, driver->minor_start, driver->num);

	kfree(devs);
	kfree(ttys);

	return res;
}

static int __init tty_init(void){
	INIT_LIST_HEAD(&drivers);
	return SUCCESS;
}

fs_initcall(tty_init);
