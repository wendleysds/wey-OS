#include "wey/printk.h"
#include <lib/list.h>
#include <lib/string.h>
#include <def/err.h>
#include <wey/blkdev.h>
#include <wey/partition.h>

struct sync_wait {
    volatile int done;
};

static void scan_endio(struct bio *bio){
    struct sync_wait *wait = bio->private;
    wait->done = 1;
}

int blk_scan_partitions(struct gendisk* disk){
	struct bio bio;
	memset(&bio, 0x0, sizeof(struct bio));

	uint8_t sec0[disk->sec_size];
	memset(sec0, 0x0, disk->sec_size);

	struct sync_wait wait = {0};

	bio.sector = 0;
	bio.len = disk->sec_size;
	bio.buffer = sec0;
	bio.op = BLK_READ;
	bio.end_io = scan_endio;
	bio.private = &wait;
	bio.next = NULL;

	blk_submit_bio(disk->bdev, &bio);

	while (!wait.done)
		asm volatile ("hlt");

	if (bio.status) {
		return bio.status;
	}

	if (sec0[510] != 0x55 || sec0[511] != 0xAA) {
		printk("Invalid MBR signature\n");
		return INVALID_FORMAT;
	}

	struct MBRPartitionEntry * parts = (struct MBRPartitionEntry *)(sec0 + 0x1BE);

	printk("MBR partitions:\n");

	for (int i = 0; i < 4; i++) {

		if (parts[i].type == 0)
			continue;

		printk("  Part %d:\n", i + 1);
		printk("    Type: 0x%x\n", parts[i].type);
		printk("    Start LBA: %u\n", parts[i].lbaFirstSector);
		printk("    Sectors: %u\n", parts[i].totalSectors);
	}

	return NOT_IMPLEMENTED;
}
