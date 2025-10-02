#include <io/ports.h>
#include <def/err.h>
#include <core/kernel.h>
#include <lib/string.h>
#include <core/sched/task.h>
#include <fs/vfs.h>
#include <drivers/ata.h>
#include <blkdev.h>
#include <device.h>

#include "ata_internal.h"

struct ATAChannel _ata_primary;
struct ATAChannel _ata_secondary;

static int _ata_open(struct inode *ino, struct file *file){
	if(!ino || !file){
		return NULL_PTR;
	}

	struct ATADevice* atadev = 0x0;
	for (int i = 0; i < 4; i++)
	{
		struct ATAChannel* channel = (i < 2) ?
			&_ata_primary : &_ata_secondary;

		atadev = &channel->devices[(i < 2 ? i : i - 2)];

		if(!atadev->exists){
			continue;
		}

		if(atadev->info.devt == ino->i_rdev){
			break;
		}

		atadev = 0x0;
	}

	if(!atadev){
		return NOT_FOUND;
	}

	file->private_data = atadev;
	
	return SUCCESS;
}

static struct file_operations fops = {
	.open = _ata_open,
	.write = ata_write,
	.read = ata_read,
	.lseek = ata_lseek
};

static int _ata_register_device(struct ATADevice* atadev, char channel){
	if(!atadev){
		return INVALID_ARG;
	}

	struct device* dev = (struct device*)kzalloc(sizeof(struct device));
	if(!dev){
		return NO_MEMORY;
	}

	struct blkdev* bdev = (struct blkdev*)kzalloc(sizeof(struct blkdev));
	if(!bdev){
		kfree(dev);
		return NO_MEMORY;
	}

	const char* names[] = {"hda", "hdb", "hdc", "hdd"};

	strncpy(dev->name, names[channel * 2 + atadev->drive], sizeof(dev->name));
	strncpy(bdev->name, (char*)atadev->model, sizeof(bdev->name));

	dev->driver_data = (void*)atadev;
	bdev->dev = dev;
	bdev->ops = &fops;

	int res = blkdev_device_add(bdev);
	if(IS_STAT_ERR(res)){
		kfree(dev);
		kfree(bdev);
		return NO_MEMORY;
	}

	atadev->info.devt = dev->devt;

	return SUCCESS;
}

static void _ata_probe_all() {
	uint16_t ioBases[] = { 0x1F0, 0x170 };
	uint16_t ctrlBases[] = { 0x3F6, 0x376 };

	for (int channel = 0; channel < 2; channel++) {
		for (int drive = 0; drive < 2; drive++) {
			struct ATAChannel* ch = (channel == 0) ?
				&_ata_primary : &_ata_secondary;

			ch->ioBase = ioBases[channel];
			ch->ctrlBase = ctrlBases[channel];

			struct ATADevice* atadev = &ch->devices[drive];
			atadev->channel = ch;
			atadev->drive = drive;

			uint16_t buffer[WORDS_PER_SECTOR];
			if (ata_identify(atadev, buffer) == SUCCESS) {
				ata_register_irq(channel);

				atadev->exists = 1;
				atadev->info.isHardDrive = buffer[0];
				atadev->UDMAmodes = buffer[88];

				atadev->info.isLBA48 = (buffer[83] & (1 << 10)) != 0;

				if (atadev->info.isLBA48) {
					atadev->addressableSectors = 
					((uint64_t)buffer[100] |
					((uint64_t)buffer[101] << 16) |
					((uint64_t)buffer[102] << 32) |
					((uint64_t)buffer[103] << 48));
				} else {
					atadev->addressableSectors = 
					((uint32_t)buffer[60] |
					((uint32_t)buffer[61] << 16));
				}

				uint8_t* dst = atadev->model;
				for (int i = 0; i < 20; i++) {
					uint16_t w = buffer[27 + i];
					*dst++ = w >> 8;
					*dst++ = w & 0xFF;
				}
				*dst = 0;

				int res = _ata_register_device(atadev, channel);

				if(IS_STAT_ERR(res)){
					warning("_ata_probe_all(): error registering dev! %d\n", res);
				}
			} else {
				atadev->exists = 0;
			}
		}
	}
}

int ata_identify(struct ATADevice* atadev, uint16_t* buffer) {
	struct ATAChannel* channel = atadev->channel;
	channel->active = atadev;

	outb_p(ATA_IO(channel, ATA_REG_HDDEVSEL), 0xA0 | (atadev->drive << 4)); // Drive/head register
	outb_p(ATA_IO(channel, ATA_REG_SECCOUNT0), 0);
	outb_p(ATA_IO(channel, ATA_REG_LBA0), 0);
	outb_p(ATA_IO(channel, ATA_REG_LBA1), 0);
	outb_p(ATA_IO(channel, ATA_REG_LBA2), 0);
	outb_p(ATA_IO(channel, ATA_REG_COMMAND), ATA_CMD_IDENTIFY);

	uint8_t status;
	if(IS_STAT_ERR(status = ata_wait_irq(atadev))){
		return status;
	}

	uint8_t cl = inb_p(ATA_IO(channel, ATA_REG_LBA1));
	uint8_t ch = inb_p(ATA_IO(channel, ATA_REG_LBA2));

	if (cl != 0 || ch != 0) return INVALID_STATE;

    insw(ATA_IO(channel, ATA_REG_DATA), buffer, WORDS_PER_SECTOR);
    return SUCCESS;
}

void ata_init(){
	memset(&_ata_primary, 0x0, sizeof(struct ATAChannel));
	memset(&_ata_secondary, 0x0, sizeof(struct ATAChannel));
	_ata_probe_all();
}
