#include <device/blkdev.h>
#include <kernel/printk.h>
#include <lib/string.h>
#include <def/errno.h>
#include <mm/kheap.h>
#include <fs/partition.h>

#define GPT_PARTITION -1

struct sync_wait {
	volatile int done;
};

static void scan_endio(struct bio *bio){
	struct sync_wait *wait = bio->private;
	wait->done = 1;
}

static int block_read(struct blkdev* dev, sector_t sector, unsigned int nr_sectors, void* buffer){
	struct bio* bio = kmalloc(sizeof(struct bio));
	if(!bio) return -ENOMEM;
	memset(bio, 0x0, sizeof(struct bio));

	struct sync_wait wait = {0};

	bio->sector = sector + dev->start_sector;
	bio->nr_sectors = nr_sectors;
	bio->buffer = buffer;
	bio->op = BLK_READ;
	bio->end_io = scan_endio;
	bio->private = &wait;
	bio->next = NULL;

	blk_submit_bio(dev, bio);

	while (!wait.done)
		asm volatile ("hlt");

	int res = bio->status;
	kfree(bio);
	return res;
}

int mbr_is_valid(void *sector0){
	uint8_t* sec0 = sector0;
	if (sec0[510] != 0x55 || sec0[511] != 0xAA) {
		return -EINVAL;
	}

	struct MBRPartitionEntry *parts = (struct MBRPartitionEntry*)(sec0 + 0x1BE);

	for (int i = 0; i < 4; i++) {
		if(parts[i].type == MBR_TYPE_GPT_PROTECT){
			return GPT_PARTITION;
		}

		// Not supported
		if(parts[i].type == MBR_TYPE_EXTENDED){
			continue;
		}

		if (parts[i].type != MBR_TYPE_EMPTY){
			return OK;
		}
	}

	return -ENOENT;
}

int gpt_is_valid(void *sector1){
	return -ENOSYS;	
}

int partition_detect(void *sector0, partition_entry_t *out_type){
	int res = mbr_is_valid(sector0);
	if(res == OK){
		*out_type = PARTITION_MBR;
		return OK;
	}

	if(res == GPT_PARTITION){
		return -ENOTSUP;
	}

	return -ENOENT; // Fallback
}

static void parse_mbr_partitions(struct gendisk* disk, uint8_t *sector0){
	int res;
	struct MBRPartitionEntry *parts = (struct MBRPartitionEntry*)(sector0 + 0x1BE);

	for(int i = 0; i < 4; i++){
		struct MBRPartitionEntry *part = &parts[i];
		if(part->type == MBR_TYPE_EXTENDED){
			continue;
		}

		if (part->type == MBR_TYPE_EMPTY){
			continue;
		}

		struct blkdev* bdev_part = (struct blkdev*)kmalloc(sizeof(struct blkdev));
		if(!bdev_part){
			res = -ENOMEM;
			goto parse_fail;
		}

		bdev_part->major = disk->major;
		bdev_part->minor = disk->first_minor + i + 1;

		bdev_part->start_sector = part->lbaStart;
		bdev_part->nr_sectors = part->totalSectors;

		bdev_part->queue = disk->bdev->queue;

		bdev_part->disk = disk;
		list_add_tail(&bdev_part->list, &disk->blkdevs);

		printk("BLK: found MBR partition \"%s%d\" (major=%u minor=%u)\n", 
			disk->name, bdev_part->minor, bdev_part->major, bdev_part->minor);

		continue;
parse_fail:
		printk("BLK-code: parse_mbr_partitions: failted to parse part \"%d\" %d", i+1, res);
	}
}

int blk_scan_partitions(struct gendisk* disk){
	uint8_t sec0[disk->sec_size];
	memset(sec0, 0x0, disk->sec_size);

	int res = block_read(disk->bdev, 0, 1, sec0);

	if(IS_ERR_VALUE(res)){
		return res;
	}

	partition_entry_t out_type;
	res = partition_detect(sec0, &out_type);

	if(IS_ERR_VALUE(res)){
		return res;
	}

	switch (out_type) {
		case PARTITION_MBR:
			parse_mbr_partitions(disk, sec0);
			break;

		default: return -ENOTSUP;
	}

	return SUCCESS;
}
