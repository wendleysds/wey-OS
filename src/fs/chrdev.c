#include <wey/device.h>
#include <wey/vfs.h>
#include <wey/chrdev.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/err.h>

static struct chrdev* chrdevs[MINOR_MAX];

void chrdev_init(){
	memset(chrdevs, 0x0, sizeof(chrdevs));
}

int chrdev_device_add(struct chrdev *cdev, struct device *dev){
	if(!cdev || !dev){
		return INVALID_ARG;
	}

	int i;
	for (i = MINOR_MAX - 1; i >= -1; i--){
		if(i == -1) return LIST_FULL;
		if(chrdevs[i] == cdev){
			return INVALID_ARG;
		}

		if(!chrdevs[i]) break;
	}
	
	dev_t devt = MKDEV(DEVCHAR_MAJOR, i);

	cdev->devt = devt;
	dev->devt = devt;

	if(IS_STAT_ERR(device_register(dev))){
		return FAILED;
	}

	chrdevs[i] = cdev;

	return SUCCESS;
}

void chrdev_device_remove(struct chrdev *cdev, struct device *dev){
	if (!cdev || !dev)
		return;

	int i;
	for (i = 0; i < MINOR_MAX; i++) {
		if (chrdevs[i] == cdev) {
			device_unregister(dev);
			chrdevs[i] = NULL;
			break;
		}
	}
}

static int chrdev_open(struct inode *ino, struct file *file){
	if (!ino || !file)
		return INVALID_ARG;

	int minor = MINOR(ino->i_rdev);
	if (minor < 0 || minor >= MINOR_MAX)
		return INVALID_ARG;

	struct chrdev *cdev = chrdevs[minor];
	if (!cdev || !cdev->ops)
		return NULL_PTR;

	file->f_op = (struct file_operations*)cdev->ops;

	if(!cdev->ops->open)
		return SUCCESS;

	return file->f_op->open(ino, file);
}

const struct file_operations def_chr_fops = {
	.open = chrdev_open,
};
