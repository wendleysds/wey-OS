#ifndef _CDEV_H
#define _CDEV_H

#include <wey/device.h>

struct file_operations;

struct chrdev{
	char name[16];
	const struct file_operations *ops;
	dev_t devt;
};

int chrdev_device_add(struct chrdev *cdev, struct device *dev);
void chrdev_device_remove(struct chrdev *cdev, struct device *dev);

#endif