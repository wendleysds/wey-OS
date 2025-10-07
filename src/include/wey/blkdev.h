#ifndef _BDEV_H
#define _BDEV_H

#include <wey/device.h>

struct file_operations;

struct blkdev{
	char name[32];
	const struct file_operations *ops;
	dev_t devt;
	struct device* dev;
};

int blkdev_device_add(struct blkdev *blkdev);
void blkdev_device_remove(struct blkdev *blkdev);

struct blkdev* blkdev_find_by_name(const char* name);

#endif