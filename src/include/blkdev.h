#ifndef _BDEV_H
#define _BDEV_H

#include <device.h>

struct file_operations;

struct blkdev{
	struct device* dev;
	const struct file_operations *ops;
	dev_t devt;
};

void blkdev_init();

int blkdev_device_add(struct blkdev *blkdev, struct device *dev);
void blkdev_device_remove(struct blkdev *blkdev, struct device *dev);

#endif