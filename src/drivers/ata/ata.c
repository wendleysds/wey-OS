#include <kernel/init.h>
#include <kernel/printk.h>
#include <device/ata.h>
#include <lib/stdio.h>
#include <def/err.h>
#include <fs/vfs.h>

#include "ata_internal.h"

struct ATAChannel _ata_primary;
struct ATAChannel _ata_secondary;

static void ata_identify_parser(struct ATADeviceInfo* ata_info, uint16_t* buffer){
	ata_info->harddrive = buffer[0];
	ata_info->supports_dma = buffer[88];
	ata_info->supports_lba48 = (buffer[83] & (1 << 10)) != 0;

	// Sectors
	if (ata_info->supports_lba48) {
		ata_info->sectors = 
		((uint64_t)buffer[100] |
		((uint64_t)buffer[101] << 16) |
		((uint64_t)buffer[102] << 32) |
		((uint64_t)buffer[103] << 48));
	} else {
		ata_info->sectors = 
		((uint32_t)buffer[60] |
		((uint32_t)buffer[61] << 16));
	}

	// Model name
	char* dst = ata_info->model;
	for (int i = 0; i < 20; i++) {
		uint16_t w = buffer[27 + i];
		*dst++ = w >> 8;
		*dst++ = w & 0xFF;
	}
	*dst = 0;

	for (int i = sizeof(ata_info->model) - 1; i >= 0; i--){
		uint8_t c = ata_info->model[i];
		if(c != ' ' && c != 0){
			ata_info->model[i+1] = 0;
			break;
		}
	}

	// Sector size
	ata_info->sec_size = 512;

	if ((buffer[106] & (1 << 14)) &&
		(buffer[106] & (1 << 12))) {

		ata_info->sec_size =
			((uint32_t)buffer[118] << 16) | buffer[117];
	}
}

static int ata_open(struct blkdev *bdev, int mode){
	if(!bdev){
		return NULL_PTR;
	}
	
	return SUCCESS;
}

static inline int ata_device(struct ATADevice* dev){
	for (int channel = 0; channel < 2; channel++) {
		for (int drive = 0; drive < 2; drive++) {
			struct ATAChannel* ch = (channel == 0) ? 
			&_ata_primary : &_ata_secondary;

			struct ATADevice* atadev = &ch->devices[drive];

			if(dev == atadev){
				return 1;
			}
		}
	}

	return 0;
}

static int ata_submit_rq(struct blkdev *bdev, struct request *req){
	struct ATADevice* atadev = bdev->disk->private;

	if(bdev->major != 3){
		return INVALID_ARG;
	}

	if(!ata_device(atadev)){
		return INVALID_ARG;
	}

	uint8_t cmd;
	switch(req->op){
		case BLK_READ: 
			cmd = atadev->info.supports_lba48 ? ATA_CMD_READ_PIO_EXT : ATA_CMD_READ_PIO;
			break;
		case BLK_WRITE: 
			cmd = atadev->info.supports_lba48 ? ATA_CMD_WRITE_PIO_EXT : ATA_CMD_WRITE_PIO;
			break;
		default: return NOT_SUPPORTED;
	}

	return ata_pio(
		atadev,
		cmd,
		req->sector,
		req->nr_sectors,
		req->bio_list->buffer
	);
}

static const struct block_device_ops ata_ops = {
	.open = ata_open,
	.submit_request = ata_submit_rq
};

static int ata_register_device(struct ATADevice *atadev, char channel){
	struct gendisk *disk = gendisk_alloc();
	if (!disk)
		return NO_MEMORY;

	int idx = channel * 2 + atadev->drive;

	snprintf(disk->name, sizeof(disk->name), "hd%c", 'a' + idx);

	disk->major = 3;
	disk->private = atadev;
	disk->ops = &ata_ops;
	disk->sec_size = atadev->info.sec_size;
	disk->capacity = atadev->info.sectors;
	disk->minors_total = 4; // 4 MBR Partitions
	
	atadev->exists = 1;
	int res = add_disk(disk);

	if(IS_ERR_VALUE(res)){
		atadev->exists = 0;
		printk("ATA: ata_register_device: error registering \"%s\"! %d\n",disk->name, res);
		kfree(disk);
	}

	return res;
}

static void ata_probe_all() {
	uint16_t ioBases[] = { 0x1F0, 0x170 };
	uint16_t ctrlBases[] = { 0x3F6, 0x376 };

	uint8_t foundDev = 0;
	int res = 0;

	for (int channel = 0; channel < 2; channel++) {
		for (int drive = 0; drive < 2; drive++) {
			struct ATAChannel* ch = (channel == 0) ?
				&_ata_primary : &_ata_secondary;

			ch->ioBase = ioBases[channel];
			ch->ctrlBase = ctrlBases[channel];
			spinlock_init(&ch->spinlock);

			struct ATADevice* atadev = &ch->devices[drive];
			atadev->channel = ch;
			atadev->drive = drive;
			INIT_LIST_HEAD(&atadev->sleepQueue);

			uint16_t buffer[WORDS_PER_SECTOR];
			if (ata_identify(atadev, buffer) == SUCCESS) {
				ata_register_irq(channel);

				if(!foundDev){
					foundDev = 1;

					res = blkdev_register(3, "ATA IDE");
					if(IS_ERR_VALUE(res)){
						printk("ATA: probe_all: error registering blkdev name! %d\n", res);
						return;
					}
				}

				ata_identify_parser(&atadev->info, buffer);
				printk("ATA: probe_all: found \"%s\"\n", atadev->info.model);

				res = ata_register_device(atadev, channel);
				if(IS_STAT_ERR(res)){
					continue;
				}

			} else {
				atadev->exists = 0;
			}
		}
	}
}

static __init int ata_init(){
	memset(&_ata_primary, 0x0, sizeof(struct ATAChannel));
	memset(&_ata_secondary, 0x0, sizeof(struct ATAChannel));

	ata_probe_all();

	return OK;
}

device_initcall(ata_init);
