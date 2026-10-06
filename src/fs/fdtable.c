#include <lib/assert.h>
#include <lib/string.h>
#include <def/errno.h>
#include <def/config.h>
#include <fs/fdtable.h>
#include <fs/vfs.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static int file_table_resize_unlocked(struct file_table* table, size_t new_capacity) {
	if (new_capacity <= table->capacity) {
		return 0;
	}

	if (new_capacity > PROC_FD_TABLE_MAX_CAPACITY) {
		new_capacity = PROC_FD_TABLE_MAX_CAPACITY;
	}

	struct file **new_files = kmalloc(sizeof(struct file*) * new_capacity);
	if (!new_files) {
		return -ENOMEM;
	}

	memset(new_files, 0, sizeof(struct file*) * new_capacity);

	if (table->files) {
		memcpy(new_files, table->files, sizeof(struct file*) * table->capacity);
		kfree(table->files);
	}

	table->files = new_files;
	table->capacity = new_capacity;

	return 0;
}

struct file_table* file_table_create(size_t capacity) {
	struct file_table* table = kmalloc(sizeof(struct file_table));
	if (!table) {
		return NULL;
	}

	memset(table, 0, sizeof(struct file_table));
	spinlock_init(&table->lock);

	if (capacity > 0) {
		if (file_table_resize_unlocked(table, capacity) < 0) {
			kfree(table);
			return NULL;
		}
	}

	return table;
}

struct file_table *file_table_clone(struct file_table *table) {
	if (!table) return NULL;

	spin_lock(&table->lock);

	struct file_table *new_table = file_table_create(table->capacity);
	if (!new_table) {
		spin_unlock(&table->lock);
		return NULL;
	}

	for (size_t i = 0; i < table->capacity; i++) {
		if (table->files[i]) {
			new_table->files[i] = table->files[i];
			file_get(new_table->files[i]);
		}
	}

	new_table->count = table->count;
	new_table->next_fd = table->next_fd;

	spin_unlock(&table->lock);
	return new_table;
}

int file_table_add_file(struct file_table* table, struct file* file) {
	if (!table || !file) return -EINVAL;

	spin_lock(&table->lock);

	int fd = -1;
	for (size_t i = table->next_fd; i < table->capacity; i++) {
		if (table->files[i] == NULL) {
			fd = i;
			break;
		}
	}

	if (fd == -1) {
		for (size_t i = 0; i < table->next_fd; i++) {
			if (table->files[i] == NULL) {
				fd = i;
				break;
			}
		}
	}

	if (fd == -1) {
		if (table->capacity >= PROC_FD_TABLE_MAX_CAPACITY) {
			spin_unlock(&table->lock);
			return -EMFILE;
		}

		size_t new_cap = MIN(table->capacity == 0 ? 8 : table->capacity * 2, PROC_FD_TABLE_MAX_CAPACITY);
		
		int res = file_table_resize_unlocked(table, new_cap);
		if (res < 0) {
			spin_unlock(&table->lock);
			return res;
		}

		fd = table->count;
	}

	table->files[fd] = file;
	table->count++;
	table->next_fd = fd + 1;

	spin_unlock(&table->lock);
	file_get(file);

	return fd;
}

void file_table_remove_file(struct file_table* table, int fd) {
	if (!table || fd < 0 || (size_t)fd >= PROC_FD_TABLE_MAX_CAPACITY) {
		return;
	}

	spin_lock(&table->lock);

	if ((size_t)fd < table->capacity && table->files[fd] != NULL) {
		struct file *f = table->files[fd];
		table->files[fd] = NULL;
		table->count--;

		if ((size_t)fd < table->next_fd) {
			table->next_fd = fd;
		}

		spin_unlock(&table->lock);

		file_put(f);
		return;
	}

	spin_unlock(&table->lock);
}

void file_table_destroy(struct file_table* table) {
	if (!table) return;

	spin_lock(&table->lock);

	for (size_t i = 0; i < table->capacity; i++) {
		if (table->files[i]) {
			file_put(table->files[i]);
		}
	}

	kfree(table->files);
	spin_unlock(&table->lock);
	kfree(table);
}
