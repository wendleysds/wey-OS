#include <kernel/device.h>
#include <kernel/init.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/err.h>
#include <fs/vfs.h>

struct chrdev{
	const struct file_operations *ops;
	struct chrdev* next;
	unsigned int major;
	unsigned int first_minor; 
	unsigned int minors_total;
	char name[32];
	struct list_head list;
} * chrdevs[MAJOR_MAX];

static spinlock_t cdev_lock;

static int find_free_major(){
	for(int i = MAJOR_MAX-1; i > 0; i--){
		if(chrdevs[i] == NULL){
			return i;
		}
	}

	return 0;
}

static inline unsigned int major_to_index(unsigned int major){
	return major % MAJOR_MAX;
}

static struct chrdev* reserve_minors_region(unsigned int major, unsigned int base_minor, unsigned int minor_total, const char *name){
	if (major >= MAJOR_MAX) {
		return ERR_PTR(INVALID_ARG);
	}

	if (minor_total > MINOR_MASK + 1 - base_minor) {
		return ERR_PTR(INVALID_ARG);
	}

	if (major == 0) {
		major = find_free_major();;
		if(major == 0){
			return ERR_PTR(LIST_FULL);
		}
	}

	struct chrdev* cdev = (struct chrdev*)kmalloc(sizeof(struct chrdev));
	if(!cdev){
		return ERR_PTR(NO_MEMORY);
	}

	int i = major_to_index(major);
	struct chrdev *cur = chrdevs[i], *prev = NULL;
	for (; cur; prev = cur, cur = cur->next) {
		if (cur->major < major)
			continue;

		if (cur->major > major)
			break;

		if (cur->first_minor + cur->minors_total <= base_minor)
			continue;

		if (cur->first_minor >= base_minor + minor_total)
			break;

		kfree(cdev);
		return ERR_PTR(LIST_FULL);
	}

	cdev->major = major;
	cdev->first_minor = base_minor;
	cdev->minors_total = minor_total;
	strncpy(cdev->name, name, sizeof(cdev->name));

	if (!prev) {
		cdev->next = cur;
		chrdevs[i] = cdev;
	} else {
		cdev->next = prev->next;
		prev->next = cdev;
	}

	return cdev;
}

static struct chrdev* unreserve_minors_region(unsigned int major, unsigned int base_minor, unsigned int minor_total){
	const unsigned int index = major_to_index(major);
	struct chrdev **current = &chrdevs[index];

	while (*current) {
		struct chrdev *entry = *current;

		const uint8_t match =
			(entry->major == major) &&
			(entry->first_minor == base_minor) &&
			(entry->minors_total == minor_total);

		if (match) {
			*current = entry->next;
			entry->next = NULL;
			return entry;
		}

		current = &entry->next;
	}

	return NULL;
}

int chardev_register(unsigned int major, unsigned int base_minor, 
	unsigned int minor_total, const char *name, const struct file_operations *fops)
{
	struct chrdev* cdev = reserve_minors_region(major, base_minor, minor_total, name);
	if(IS_ERR_VALUE(cdev)){
		return PTR_ERR(cdev);
	}

	cdev->ops = fops;
	return major ? 0 : cdev->major;
}

void chardev_unregister(unsigned int major, unsigned int base_minor,
	unsigned int minor_total, const char *name)
{
	struct chrdev* cdev = unreserve_minors_region(major, base_minor, minor_total);
	if(!cdev){
		return;
	}

	if(strncmp(cdev->name, name, sizeof(cdev->name)) != 0){
		return;
	}

	kfree(cdev);
}

static int chrdev_open(struct inode *ino, struct file *file){


	return NOT_IMPLEMENTED;
}

const struct file_operations def_chr_fops = {
	.open = chrdev_open,
};

static int __init chrdev_init(){
	memset(chrdevs, 0x0, sizeof(chrdevs));
	spinlock_init(&cdev_lock);
	return SUCCESS;
}

core_initcall(chrdev_init);
