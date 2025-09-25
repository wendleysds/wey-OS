#include <device.h>
#include <def/config.h>
#include <def/status.h>
#include <core/kernel.h>
#include <lib/string.h>

struct device* devices[DEVICES_MAX] = { 0 };

int __must_check device_register(struct device *dev){
	int freeIndex = -1;
    for (int i = 0; i < DEVICES_MAX; i++){
		if(dev->id == devices[i]->id || dev->devt == devices[i]->devt){
			return INVALID_ARG;
		}

        if(!devices[i] && freeIndex == -1){
           freeIndex = i;
        }
    }

	if(freeIndex != -1){
		devices[freeIndex] = dev;
		return SUCCESS;
	}

    warning("device_register(): device '%s' not registered! max devices reached\n", dev->name);

    return LIST_FULL;
}

int device_unregister(struct device *dev){
    if(devices[dev->id]->id == dev->id){
        devices[dev->id] = 0x0;
        return SUCCESS;
    }

    for (int i = 0; i < DEVICES_MAX; i++){
        struct device* d = devices[i];
        if(d->id == dev->id){
            d->id = -1;
            devices[i] = 0x0;
            return SUCCESS;
        }
    }

    return NOT_FOUND;
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