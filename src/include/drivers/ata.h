#ifndef _ATA_LBA_H
#define _ATA_LBA_H

#include <wey/device.h>
#include <stdint.h>

struct TaskQueue;
struct ATAChannel;

struct ATADevice {
	uint8_t exists;
	uint8_t model[41];
	int8_t drive;

	struct {
		int8_t isLBA48;
		int8_t isHardDrive;
		dev_t devt;
	} info;

	uint16_t UDMAmodes;
	uint64_t addressableSectors;

	volatile char irqTriggered;

	struct ATAChannel* channel;
	struct TaskQueue sleepQueue;
};

struct ATAChannel{
	uint16_t ioBase;    // 0x1F0 or 0x170
	uint16_t ctrlBase;  // 0x3F6 or 0x376
	struct ATADevice* active;
	struct ATADevice devices[2];
};

void ata_init();
int ata_identify(struct ATADevice* dev, uint16_t* buffer);
int ata_flush(struct ATADevice* dev);

#endif

