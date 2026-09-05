#include <device/tty.h>
#include <kernel/init.h>
#include <kernel/sched.h>
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
extern const struct file_operations tty_file_ops;

static LIST_HEAD(drivers);

static const struct termios termios_default = { 
	.c_iflag = ICRNL,
	.c_oflag = OPOST,
	.c_cflag = CREAD | CS8,
	.c_lflag = ECHO | ICANON | ECHOE | ECHOK,
	.c_cc = {
		[VINTR] = 0x03,
		[VQUIT] = 0x1c,
		[VERASE] = 0x8, //0x7F,
		[VKILL] = 0x15,
		[VEOF] = 0x04,
		[VTIME] = 0x00,
		[VMIN] = 0x01,
		[VSWTC] = 0x00,
		[VSTART] = 0x11,
		[VSTOP] = 0x13,
		[VSUSP] = 0x1a,
		[VEOL] = 0x00,
		[VREPRINT] = 0x12,
		[VDISCARD] = 0x0f,
		[VWERASE] = 0x17,
		[VLNEXT] = 0x16,
		[VEOL2] = 0x00,
	} 
};

struct tty_driver* tty_alloc_driver(){
	struct tty_driver* driver = kzalloc(sizeof(struct tty_driver));
	if(driver){
		INIT_LIST_HEAD(&driver->list);
	}
	return driver;
}

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
		wait_queue_head_init(&tty->read_waiters);
		tty->termios = termios_default;

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

		res = tty_bufhead_init(&tty->buffer);
		if(res < 0){
			tty_ldisc_ops.close(tty);
			kfree(tty);
			return ERR_PTR(res);
		}

		driver->ttys[index] = tty;
		tty->ops = tty->driver->ops;
	}

	return tty;
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
