#include <wey/device.h>
#include <wey/vfs.h>
#include <wey/blkdev.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/err.h>

static struct blkdev* blkdevs[MINOR_MAX];

void blkdev_init(){
	memset(blkdevs, 0x0, sizeof(blkdevs));
}

int blkdev_device_add(struct blkdev *blkdev){
	if(!blkdev || !blkdev->dev){
		return INVALID_ARG;
	}

	int i;
	for (i = MINOR_MAX - 1; i >= -1; i--){
		if(i == -1) return LIST_FULL;
		if(blkdevs[i] == blkdev){
			return INVALID_ARG;
		}

		if(!blkdevs[i]) break;
	}
	
	dev_t devt = MKDEV(DEVBLOCK_MAJOR, i);

	blkdev->devt = devt;
	blkdev->dev->devt = devt;

	if(IS_STAT_ERR(device_register(blkdev->dev))){
		return FAILED;
	}

	blkdevs[i] = blkdev;

	return SUCCESS;
}

void blkdev_device_remove(struct blkdev *blkdev){
	if(!blkdev){
		return;
	}

	blkdevs[MINOR(blkdev->devt)] = 0x0;
	device_unregister(blkdev->dev);
}

static int blkdev_open(struct inode *ino, struct file *file){
	if (!ino || !file)
		return INVALID_ARG;

	int minor = MINOR(ino->i_rdev);
	if (minor < 0 || minor >= MINOR_MAX)
		return INVALID_ARG;

	struct blkdev *blkdev = blkdevs[minor];
	if (!blkdev || !blkdev->ops)
		return NULL_PTR;

	file->f_op = blkdev->ops;

	if(!blkdev->ops->open)
		return SUCCESS;

	return file->f_op->open(ino, file);
}

struct blkdev* blkdev_find_by_name(const char* name){
	if(!name) return ERR_PTR(NULL_PTR);

	for (int i = 0; i < MINOR_MAX; i++){
		if(blkdevs[i]){
			if(strcmp(blkdevs[i]->dev->name, name) == 0){
				return blkdevs[i];
			}
		}
	}
	
	return ERR_PTR(NOT_FOUND);
}

const struct file_operations def_blk_fops = {
	.open = blkdev_open,
};
