#include <lib/list.h>
#include <wey/device.h>
#include <wey/vfs.h>
#include <wey/blkdev.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/err.h>
#include <def/init.h>

static struct blk_major_name{
	char name[32];
	int major;
	struct blk_major_name *next;
} * major_names[MAJOR_MAX];

static inline unsigned int major_to_index(unsigned int major){
	return major % MAJOR_MAX;
}

static int find_free_major(){
	for(int i = MAJOR_MAX-1; i > 0; i--){
		if(major_names[i] == NULL){
			return i;
		}
	}

	return 0;
}

int blkdev_register(unsigned int major, const char* name){
	if (major >= MAJOR_MAX) {
		return INVALID_ARG;
	}

	int ret = SUCCESS;
	struct blk_major_name** cur, *entry;

	if(major == 0){
		major = find_free_major();
		if(major == 0){
			return LIST_FULL;
		}

		ret = major;
	}

	entry = kmalloc(sizeof(struct blk_major_name));
	if(!entry){
		ret = NO_MEMORY;
		goto out;
	}

	strncpy(entry->name, name, sizeof(entry->name));
	entry->major = major;

	for (cur = &major_names[major_to_index(major)]; *cur; cur = &(*cur)->next) {
		if ((*cur)->major == major){
			break;
		}
	}

	if (!*cur){
		*cur = entry;
		goto out;
	}
		
	ret = LIST_FULL;
	kfree(entry);

out:
	return ret;
}

void blkdev_unregister(unsigned int major, const char *name){
	struct blk_major_name** cur, * entry;

	if (!name)
		return;

	if (major >= MAJOR_MAX)
		return;

	cur = &major_names[major_to_index(major)];

	while (*cur) {
		entry = *cur;

		if (entry->major == major && strcmp(entry->name, name) == 0) {
			*cur = entry->next;
			kfree(entry);
			return;
		}

		cur = &entry->next;
	}
}

int add_disk(struct gendisk* disk){
	if (!disk)
		return INVALID_ARG;

	if (disk->major >= MAJOR_MAX)
		return INVALID_ARG;

	if (!major_names[major_to_index(disk->major)])
		return INVALID_ARG;

	if (disk->minors_total <= 0)
		return INVALID_ARG;

	INIT_LIST_HEAD(&disk->blkdevs);

	struct blkdev *bdev = kmalloc(sizeof(struct blkdev));
	if (!bdev){
		return NO_MEMORY;
	}

	struct request_queue *queue = kmalloc(sizeof(struct request_queue));
	if (!bdev){
		kfree(bdev);
		return NO_MEMORY;
	}

	queue->head = queue->tail = NULL;
	spinlock_init(&queue->lock);
	queue->elevator = &fifo_elevator;
	queue->bdev = bdev;

	bdev->major = disk->major;
	bdev->minor = disk->first_minor;
	bdev->start_sector = 0;
	bdev->nr_sectors = disk->capacity;
	bdev->disk = disk;
	bdev->queue = queue;
	disk->bdev = bdev;

	list_add_tail(&bdev->list, &disk->blkdevs);

	int res = blk_scan_partitions(disk);
	if(IS_ERR_VALUE(res)){
		kfree(bdev);
		kfree(queue);
		return res;
	}

	return SUCCESS;
}

void remove_disk(struct gendisk* disk){
    struct blkdev *pos;
    list_for_each_entry(pos, &disk->blkdevs, list) {
        list_remove(&pos->list);
        kfree(pos);
    }
}

static int blkdev_open(struct inode *ino, struct file *file){
	return NOT_IMPLEMENTED;
}

struct blkdev* blkdev_find_by_name(const char* name){
	return ERR_PTR(NOT_IMPLEMENTED);
}

struct gendisk* gendisk_alloc(){
	return kzalloc(sizeof(struct gendisk));
}

const struct file_operations def_blk_fops = {
	.open = blkdev_open,
};

static __init int blkdev_init(){
	memset(major_names, 0x0, sizeof(major_names));
	return OK;
}

core_initcall(blkdev_init);
