#ifndef _DEVICE_H
#define _DEVICE_H

#include <def/compile.h>
#include <lib/list.h>
#include <stdint.h>

#define DEVMEM_MAJOR    1
#define DEVCHAR_MAJOR   2
#define DEVBLOCK_MAJOR  3

#define MINOR_BITS 20
#define MINOR_MASK ((1U << MINOR_BITS) - 1)

#define MKDEV(ma,mi) (((ma) << MINOR_BITS) | (mi))
#define MINOR(devt) ((unsigned int)((devt) & MINOR_MASK))
#define MAJOR(devt) ((unsigned int)((devt) >> MINOR_BITS))

typedef unsigned int dev_t;

struct device {
	uint16_t id;
	uint8_t type;
	char name[16];

	uint32_t mode;
	uint8_t flags;

	void* driver_data;

	dev_t devt;

	struct list_head list;

	// TMP ops
	int (*read)(struct device* dev, void* buffer, uint32_t size, uint64_t offset);
	int (*write)(struct device* dev, const void* buffer, uint32_t size, uint64_t offset);
	long (*ioctl)(struct device* dev, unsigned int cmd, unsigned long arg);
	int (*close)(struct device* dev);
};

void device_init();

int __must_check device_register(struct device *dev);
void device_unregister(struct device *dev);

struct device* device_get_name(const char* name);

#endif