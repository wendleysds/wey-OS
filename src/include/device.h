#ifndef _DEVICE_H
#define _DEVICE_H

#include <def/compile.h>
#include <stdint.h>

#define DEVICE_BLOCK 1
#define DEVICE_CHAR  2

struct device {
    uint16_t id;
    uint8_t type;
    char name[16];

    uint32_t mode;
    uint8_t flags;

    void* driver_data;

    int (*read)(struct device* dev, void* buffer, uint32_t size, uint64_t offset);
    int (*write)(struct device* dev, const void* buffer, uint32_t size, uint64_t offset);
    int (*ioctl)(struct device* dev, uint32_t request, ...);
    int (*close)(struct device* dev);
};

int __must_check device_register(struct device *dev);
int device_unregister(struct device *dev);
struct device* device_get_name(const char* name);
struct device* device_get_id(uint16_t id);

#endif