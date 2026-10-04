#include <kernel/device.h>
#include <kernel/resource.h>
#include <kernel/init.h>
#include <mm/kheap.h>
#include <lib/string.h>
#include <def/errno.h>

#define MAX_DEVICE_NAME 64

static LIST_HEAD(devices);
static LIST_HEAD(busses);

static int duplicate_device(dev_t dev){
	struct device* pos;
	list_for_each_entry(pos, &devices, node){
		if(pos->devt == dev){
			return 1;
		}
	}

	return 0;
}

void bus_register(struct bus_type *bus){
	if (!bus)
		return;

	INIT_LIST_HEAD(&bus->node);
	INIT_LIST_HEAD(&bus->device_list);
	INIT_LIST_HEAD(&bus->driver_list);

	list_add_tail(&bus->node, &busses);
}

void bus_unregister(struct bus_type *bus){
	if (!bus)
		return;
	
	list_remove(&bus->node);
}

struct bus_type* bus_find_by_name(const char *name){
	struct bus_type *pos;
	list_for_each_entry(pos, &busses, node) {
		if (strcmp(pos->name, name) == 0) {
			return pos;
		}
	}
	return NULL;
}

static int bus_probe_device(struct device *dev) {
	if (dev->driver) return 0;

	struct device_driver *drv;
	list_for_each_entry(drv, &dev->bus->driver_list, node) {
		if (dev->bus->match(dev, drv)) {
			dev->driver = drv;
			int ret = drv->probe(dev);
			if (ret == 0) {
				return 0;
			}
			dev->driver = NULL;
		}
	}
	return -ENODEV;
}

void driver_register(struct device_driver *drv){
	if (!drv || !drv->bus) return;

	INIT_LIST_HEAD(&drv->node);
	
	if(drv->bus){
		list_add_tail(&drv->node, &drv->bus->driver_list);

		struct device *dev;
		list_for_each_entry(dev, &drv->bus->device_list, bus_list) {
			if (!dev->driver && drv->bus->match(dev, drv)) {
				dev->driver = drv;
				int ret = drv->probe(dev);
				if (ret != 0) {
					dev->driver = NULL;
				}
			}
		}
	}
}

void driver_unregister(struct device_driver *drv){
	if (!drv) return;

	if(drv->bus){
		list_remove(&drv->node);
	}
}

void device_initialize(struct device *dev){
	memset(dev, 0x0, sizeof(struct device));
	INIT_LIST_HEAD(&dev->node);
	INIT_LIST_HEAD(&dev->bus_list);
	dev->resources = NULL;
}

int device_register(struct device *dev){
	if(!dev) return -EINVAL;

	if(dev->devt != 0 && duplicate_device(dev->devt)){
		return -EEXIST;
	}

	list_add_tail(&dev->node, &devices);
	if(dev->bus){
		list_add_tail(&dev->bus_list, &dev->bus->device_list);
		bus_probe_device(dev);
	}

	return SUCCESS;
}

void device_unregister(struct device *dev){
	if(!dev){
		return;
	}

	if(list_empty(&dev->node)){
		return;
	}

	list_remove(&dev->node);
}

struct device* device_create(dev_t devt, void *drvdata, const char *name){
	if(devt == 0 || !name){
		return ERR_PTR(-EINVAL); 
	}

	struct device* dev = kmalloc(sizeof(struct device));
	if(!dev){
		return ERR_PTR(-ENOMEM);
	}

	size_t name_len = strnlen(name, MAX_DEVICE_NAME);
	if(name_len >= MAX_DEVICE_NAME){
		kfree(dev);
		return ERR_PTR(-ENAMETOOLONG);
	}

	if(name[name_len] != '\0'){
		kfree(dev);
		return ERR_PTR(-EINVAL);
	}

	char* s = strdup(name);
	if(!s){
		kfree(dev);
		return ERR_PTR(-ENOMEM);
	}

	s[name_len] = '\0';

	device_initialize(dev);

	dev->name = s;
	dev->devt = devt;
	dev->driver_data = drvdata;

	int res = device_register(dev);
	if(res){
		kfree(s);
		kfree(dev);
		return ERR_PTR(res);
	}

	return dev;
}

struct device* device_get_by_name(const char* name){
	if(!name){
		return NULL;
	}

	struct device* pos;
	list_for_each_entry(pos, &devices, node){
		if(strcmp(pos->name, name) == 0){
			return pos;
		}
	}

	return NULL;
}

struct device* device_get_by_devt(dev_t devt){
	struct device* pos;
	list_for_each_entry(pos, &devices, node){
		if(pos->devt == devt){
			return pos;
		}
	}

	return NULL;
}

void device_add_resource(struct device *dev, struct resource *res)
{
	if (!dev || !res)
		return;

	res->sibling = NULL;

	if (!dev->resources) {
		dev->resources = res;
		return;
	}

	struct resource *cur = dev->resources;
	while (cur->sibling){
		cur = cur->sibling;
	}

	cur->sibling = res;
}

struct resource *device_get_resource(
	struct device *dev,
	resource_type_t type,
	unsigned int index
){
	if (!dev)
		return NULL;

	unsigned int n = 0;
	for (struct resource *r = dev->resources; r; r = r->sibling) {
		if (r->type == type) {
			if (n == index)
				return r;
			n++;
		}
	}
	return NULL;
}

static int __init device_init(){
	INIT_LIST_HEAD(&devices);
	return SUCCESS;
}

core_initcall(device_init);
