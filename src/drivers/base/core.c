#include <device.h>
#include <def/config.h>
#include <def/status.h>
#include <core/kernel.h>
#include <lib/string.h>
#include <lib/mem.h>

extern void chrdev_init();
extern void blkdev_init();

struct device* devices[DEVICES_MAX] = { 0 };
uint16_t next_id = 1;

void device_init(){
	blkdev_init();
	chrdev_init();

	memset(devices, 0x0, sizeof(devices));
	next_id = 1;
}

int __must_check device_register(struct device *dev){
	int freeIndex = -1;
    for (int i = 0; i < DEVICES_MAX; i++){
		if(devices[i]){
			if(dev->id == devices[i]->id || dev->devt == devices[i]->devt){
				return INVALID_ARG;
			}
		}else if(freeIndex == -1){
			freeIndex = i;
		}
    }

	if(freeIndex != -1){
		devices[freeIndex] = dev;
		dev->id = next_id++;
		return SUCCESS;
	}

    warning("device_register(): device '%s' not registered! max devices reached\n", dev->name);

    return LIST_FULL;
}

void device_unregister(struct device *dev){
    for (int i = 0; i < DEVICES_MAX; i++){
        struct device* d = devices[i];
        if(d->id == dev->id){
            devices[i] = 0x0;
            return;
        }
    }
}

struct device* device_get_name(const char* name){
    for (int i = 0; i < DEVICES_MAX; i++){
        struct device* d = devices[i];
        if(strcmp(d->name, name) == 0){
            return d;
        }
    }

    return 0x0;
}

struct device* device_get_id(uint16_t id){
    for (int i = 0; i < DEVICES_MAX; i++){
        struct device* d = devices[i];
        if(d->id == id){
            return d;
        }
    }

    return 0x0;
}