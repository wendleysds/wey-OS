#include <device/blkdev.h>
#include <kernel/printk.h>
#include <kernel/init.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/errno.h>
#include <fs/vfs.h>

static struct blk_major_name{
	char name[32];
	int major;
	struct blk_major_name *next;
} * major_names[MAJOR_MAX];

static struct list_head disks[MAJOR_MAX];

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
		return -EINVAL;
	}

	int ret = SUCCESS;
	struct blk_major_name** cur, *entry;

	if(major == 0){
		major = find_free_major();
		if(major == 0){
			return -ENOENT;
		}

		ret = major;
	}

	entry = kmalloc(sizeof(struct blk_major_name));
	if(!entry){
		ret = -ENOMEM;
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
		
	ret = -ENOENT;
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
		return -EINVAL;

	if (disk->major >= MAJOR_MAX)
		return -EINVAL;

	if (!major_names[major_to_index(disk->major)])
		return -EINVAL;

	if (disk->minors_total <= 0)
		return -EINVAL;

	INIT_LIST_HEAD(&disk->blkdevs);
	INIT_LIST_HEAD(&disk->list);

	struct blkdev *bdev = kmalloc(sizeof(struct blkdev));
	if (!bdev)
		return -ENOMEM;

	struct request_queue *queue = kmalloc(sizeof(struct request_queue));
	if (!queue) {
		kfree(bdev);
		return -ENOMEM;
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

	INIT_LIST_HEAD(&bdev->list);
	list_add_tail(&disk->list, &disks[disk->major]);
	list_add_tail(&bdev->list, &disk->blkdevs);

	printk("BLK: added disk \"%s\" (major=%u minor=%u)\n",
		disk->name, disk->major, disk->first_minor);

	int res = blk_scan_partitions(disk);
	if (IS_ERR_VALUE(res) && res != -ENOENT) {
		printk("BLK-core: failed to read \"%s\" partitions %d\n",
				disk->name, res);
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
	return -ENOSYS;
}

struct blkdev* blk_find_by_name(const char* name){
	return ERR_PTR(-ENOSYS);
}

struct blkdev* blk_lookup(unsigned int major, unsigned int minor){
	struct gendisk* disk;
	struct blkdev* dev;
	list_for_each_entry(disk, &disks[major], list){
		if (minor >= disk->first_minor && minor < disk->first_minor + disk->minors_total){
			unsigned int partno = minor - disk->first_minor;
			if (partno == 0){
				return disk->bdev;
			}
				
			list_for_each_entry(dev, &disk->blkdevs, list){
				if(dev->minor == minor){
					return dev;
				}
			}

			return NULL;
		}
	}

	return NULL;
}

struct gendisk* gendisk_alloc(){
	return kzalloc(sizeof(struct gendisk));
}

const struct file_operations def_blk_fops = {
	.open = blkdev_open,
};

static __init int blkdev_init(){
	memset(major_names, 0x0, sizeof(major_names));
	for (int i = 0; i < MAJOR_MAX; i++){
    	INIT_LIST_HEAD(&disks[i]);
	}

	return OK;
}

core_initcall(blkdev_init);
