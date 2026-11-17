#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <lib/string.h>
#include <fs/fdtable.h>
#include <def/errno.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define INITIAL_FD_CAPACITY 8

static inline bool is_valid_fd(int fd) {
	return fd >= 0 && (size_t)fd < PROC_FD_TABLE_MAX_CAPACITY;
}

static int file_table_resize_unlocked(struct file_table *table, size_t min_capacity) {
	if (min_capacity <= table->capacity) {
		return 0;
	}

	if (min_capacity > PROC_FD_TABLE_MAX_CAPACITY) {
		return -EMFILE;
	}

	size_t new_capacity = table->capacity == 0 ? INITIAL_FD_CAPACITY : table->capacity * 2;
	new_capacity = MAX(new_capacity, min_capacity);
	new_capacity = MIN(new_capacity, PROC_FD_TABLE_MAX_CAPACITY);

	struct file **new_files = kmalloc(sizeof(struct file *) * new_capacity);
	if (!new_files) {
		return -ENOMEM;
	}

	memset(new_files, 0, sizeof(struct file *) * new_capacity);

	if (table->files) {
		memcpy(new_files, table->files, sizeof(struct file *) * table->capacity);
		kfree(table->files);
	}

	table->files = new_files;
	table->capacity = new_capacity;

	return 0;
}

static int file_table_find_free_fd_unlocked(struct file_table *table) {
	for (size_t i = table->next_fd; i < table->capacity; i++) {
		if (!table->files[i]) {
			return (int)i;
		}
	}

	for (size_t i = 0; i < table->next_fd; i++) {
		if (!table->files[i]) {
			return (int)i;
		}
	}

	return -1;
}

struct file_table *file_table_create(size_t capacity) {
	struct file_table *table = kmalloc(sizeof(struct file_table));
	if (!table) {
		return NULL;
	}

	memset(table, 0, sizeof(struct file_table));
	spinlock_init(&table->lock);

	if (capacity > 0 && file_table_resize_unlocked(table, capacity) < 0) {
		kfree(table);
		return NULL;
	}

	return table;
}

struct file_table *file_table_clone(struct file_table *table) {
	if (!table) {
		return NULL;
	}

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

int file_table_add_file(struct file_table *table, struct file *file) {
	if (!table || !file) {
		return -EINVAL;
	}

	spin_lock(&table->lock);

	int fd = file_table_find_free_fd_unlocked(table);
	if (fd < 0) {
		int res = file_table_resize_unlocked(table, table->capacity + 1);
		if (res < 0) {
			spin_unlock(&table->lock);
			return res;
		}
		fd = (int)table->count;
	}

	file_get(file);
	table->files[fd] = file;
	table->count++;
	table->next_fd = (size_t)fd + 1;

	spin_unlock(&table->lock);
	return fd;
}

int file_table_install_fd(struct file_table *table, int fd, struct file *file) {
	if (!table || !file || !is_valid_fd(fd)) {
		return -EBADF;
	}

	spin_lock(&table->lock);

	int res = file_table_resize_unlocked(table, (size_t)fd + 1);
	if (res < 0) {
		spin_unlock(&table->lock);
		return res;
	}

	struct file *old_file = table->files[fd];
	if (!old_file) {
		table->count++;
	}

	file_get(file);
	table->files[fd] = file;

	spin_unlock(&table->lock);

	if (old_file) {
		file_put(old_file);
	}

	return fd;
}

int file_table_remove_file(struct file_table *table, int fd) {
	if (!table || !is_valid_fd(fd)) {
		return -EBADF;
	}

	spin_lock(&table->lock);

	if ((size_t)fd >= table->capacity || !table->files[fd]) {
		spin_unlock(&table->lock);
		return -EBADF;
	}

	struct file *f = table->files[fd];
	table->files[fd] = NULL;
	table->count--;

	if ((size_t)fd < table->next_fd) {
		table->next_fd = (size_t)fd;
	}

	spin_unlock(&table->lock);

	file_put(f);
	return 0;
}

void file_table_destroy(struct file_table *table) {
	if (!table) {
		return;
	}

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

struct file *file_table_get_file(struct file_table *table, int fd) {
	if (!table || !is_valid_fd(fd)) {
		return NULL;
	}

	struct file *found = NULL;

	spin_lock(&table->lock);
	if ((size_t)fd < table->capacity && table->files[fd]) {
		found = table->files[fd];
		file_get(found);
	}
	spin_unlock(&table->lock);

	return found;
}

SYSCALL_DEFINE1(dup, int, oldfd) {
	struct file_table *table = current->files;
	struct file *f = file_table_get_file(table, oldfd);
	if (!f) {
		return -EBADF;
	}

	int fd = file_table_add_file(table, f);
	file_put(f);
	return fd;
}

SYSCALL_DEFINE2(dup2, int, oldfd, int, newfd) {
	struct file_table *table = current->files;
	struct file *f = file_table_get_file(table, oldfd);
	if (!f) {
		return -EBADF;
	}

	if (oldfd == newfd) {
		file_put(f);
		return newfd;
	}

	int res = file_table_install_fd(table, newfd, f);
	file_put(f);
	return res;
}