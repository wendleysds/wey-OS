#include <kernel/device.h>
#include <kernel/init.h>
#include <lib/string.h>
#include <def/errno.h>

static LIST_HEAD(devices);

static int next_id = 1;

static int duplicate_device(dev_t dev){
	struct device* pos;
	list_for_each_entry(pos, &devices, list){
		if(pos->devt == dev){
			return 1;
		}
	}

	return 0;
}

int __must_check device_register(struct device *dev){
	if(!dev){
		return -EINVAL;
	}

	if(duplicate_device(dev->devt)){
		return -EEXIST;
	}

	INIT_LIST_HEAD(&dev->list);

	dev->id = next_id++;
	list_add_tail(&dev->list, &devices);

	return SUCCESS;
}

void device_unregister(struct device *dev){
	if(!dev){
		return;
	}

	if(dev->id == 0){
		return;
	}

	if(list_empty(&dev->list)){
		return;
	}

	dev->id = 0;
	list_remove(&dev->list);
}

struct device* device_get_by_name(const char* name){
	if(!name){
		return NULL;
	}

	struct device* pos;
	list_for_each_entry(pos, &devices, list){
		if(strcmp(pos->name, name) == 0){
			return pos;
		}
	}

	return NULL;
}
struct device* device_get_by_devt(dev_t devt){
	struct device* pos;
	list_for_each_entry(pos, &devices, list){
		if(pos->devt == devt){
			return pos;
		}
	}

	return NULL;
}

struct device* device_get_by_id(int id){
	struct device* pos;
	list_for_each_entry(pos, &devices, list){
		if(pos->id == id){
			return pos;
		}
	}

	return NULL;
}

static int __init device_init(){
	next_id = 1;
	INIT_LIST_HEAD(&devices);
	return SUCCESS;
}

core_initcall(device_init);
