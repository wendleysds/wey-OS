#ifndef _ATA_LBA_H
#define _ATA_LBA_H

#include <kernel/device.h>
#include <sync/spinlock.h>
#include <lib/string.h>
#include <stdint.h>

struct ATAChannel;

struct ATADeviceInfo {
	char harddrive;
	uint64_t sectors;
	uint16_t sec_size;
	int supports_lba48;
	int supports_dma;
	int supports_ncq;
	char model[41];
};

struct ATADevice {
	uint8_t exists;
	uint8_t drive;

	dev_t devt;
	struct ATADeviceInfo info;
	volatile char irqTriggered;

	struct ATAChannel* channel;
	struct list_head sleepQueue;
};

struct ATAChannel{
	uint16_t ioBase;    // 0x1F0 or 0x170
	uint16_t ctrlBase;  // 0x3F6 or 0x376
	struct ATADevice* active;
	struct ATADevice devices[2];
	spinlock_t spinlock;
	char irqRegistered;
};

int ata_identify(struct ATADevice* dev, uint16_t* buffer);
int ata_flush(struct ATADevice* dev);

#endif

