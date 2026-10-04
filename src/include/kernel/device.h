#ifndef _DEVICE_H
#define _DEVICE_H

#include <def/compile.h>
#include <lib/list.h>
#include <sys/types.h>

#define MINOR_BITS 20
#define MINOR_MASK ((1U << MINOR_BITS) - 1)

#define MKDEV(ma,mi) (((ma) << MINOR_BITS) | (mi))
#define MINOR(devt) ((unsigned int)((devt) & MINOR_MASK))
#define MAJOR(devt) ((unsigned int)((devt) >> MINOR_BITS))

struct device;
struct bus_type;
struct device_driver;

struct bus_type {
	const char* name;

	int (*match)(struct device *dev, struct device_driver *drv);

	struct list_head device_list;
	struct list_head driver_list;
	struct list_head node;
};

struct device_driver {
	const char* name;
	struct bus_type *bus;

	int (*probe)(struct device *dev);
	int (*remove)(struct device *dev);
	struct list_head node;
};

/* Base struct for all devices */
struct device {
	const char* name;

	struct bus_type *bus;
	struct device_driver *driver;

	struct device *parent;

	void *driver_data;
	void *bus_data;

	dev_t devt;

	struct resource *resources;

	struct list_head bus_list;
	struct list_head node;
};

void device_initialize(struct device *dev);
int device_register(struct device *dev);
void device_unregister(struct device *dev);

void bus_register(struct bus_type *bus);
void bus_unregister(struct bus_type *bus);
struct bus_type* bus_find_by_name(const char *name);

void driver_register(struct device_driver *driver);
void driver_unregister(struct device_driver *driver);

struct device *device_create    (dev_t devt, void *drvdata, const char *name);
struct device *device_get_by_name(const char *name);
struct device *device_get_by_devt(dev_t devt);

#endif