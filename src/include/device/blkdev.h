#ifndef _BLKDEV_H
#define _BLKDEV_H

#include <sync/spinlock.h>
#include <lib/list.h>
#include <stdint.h>

#define BLK_READ 0
#define BLK_WRITE 1

struct file_operations;
struct elevator_ops;
struct blkdev;
struct bio;

/*1 sector = 1 LBA*/
typedef uint64_t sector_t;

typedef void (bio_end_io_t) (struct bio*);

struct bio {
	sector_t sector;
	struct blkdev* bdev;
	unsigned int nr_sectors;
	void *buffer;
	void *private;

	int status;
	int op;

	bio_end_io_t *end_io;

	struct bio *next;
};

struct request {
	sector_t sector;
	unsigned int nr_sectors;

	int status;
	int op;

	struct bio *bio_list;
	struct request_queue *q;
	struct request* next;
};

struct request_queue {
	struct request *head;
	struct request *tail;

	spinlock_t lock;

	const struct elevator_ops *elevator;

	struct blkdev *bdev;
};

struct elevator_ops {
	void (*insert)(struct request_queue *, struct request *);
	struct request *(*dispatch)(struct request_queue *);
};

struct block_device_ops {
	int  (*open)(struct blkdev *bdev, int mode);
	void (*release)(struct blkdev *bdev);
	int  (*submit_request)(struct blkdev *bdev, struct request *req);
	int  (*ioctl)(struct blkdev *bdev, unsigned int cmd, unsigned long arg);
	int  (*flush)(struct blkdev *bdev);
};

struct gendisk {
	int major;
	int first_minor;
	int minors_total;

	char name[32];
	uint64_t capacity;
	uint16_t sec_size;

	const struct block_device_ops *ops;
	struct blkdev *bdev;

	void *private;
	struct list_head blkdevs;
	struct list_head list;
};

struct blkdev {
	unsigned int major;
	unsigned int minor;

	sector_t start_sector;
	sector_t nr_sectors;

	struct request_queue *queue;

	struct gendisk *disk;
	struct device *bdev;
	struct list_head list;
};

extern struct elevator_ops fifo_elevator;

int blkdev_register(unsigned int major, const char* name);
void blkdev_unregister(unsigned int major, const char* name);

int add_disk(struct gendisk* disk);
void remove_disk(struct gendisk* disk);

struct gendisk* gendisk_alloc();

struct blkdev* blk_find_by_name(const char* name);
struct blkdev* blk_lookup(unsigned int major, unsigned int minor);
int blk_submit_bio(struct blkdev *bdev, struct bio *bio);
void blk_run_queue(struct request_queue *q);

int blk_scan_partitions(struct gendisk* disk);
void blk_complete_request(struct request *rq, int status);

#endif