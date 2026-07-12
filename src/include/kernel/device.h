#ifndef _DEVICE_H
#define _DEVICE_H

#include <def/compile.h>
#include <lib/list.h>
#include <sys/types.h>

enum device_type {
	DEVICE_CLASS_NONE = 0,
	DEVICE_CLASS_BLOCK,
	DEVICE_CLASS_CHAR,
	DEVICE_CLASS_TTY,
};

#define MINOR_BITS 20
#define MINOR_MASK ((1U << MINOR_BITS) - 1)

#define MKDEV(ma,mi) (((ma) << MINOR_BITS) | (mi))
#define MINOR(devt) ((unsigned int)((devt) & MINOR_MASK))
#define MAJOR(devt) ((unsigned int)((devt) >> MINOR_BITS))

// Base struct for all devices
struct device {
	int id;
	const char* name;
	enum device_type type;

	void* driver_data;

	dev_t devt;

	struct list_head list;
};

int __must_check device_register(struct device *dev);
void device_unregister(struct device *dev);

struct device* device_get_by_name(const char* name);
struct device* device_get_by_devt(dev_t devt);
struct device* device_get_by_id(int id);

#endif