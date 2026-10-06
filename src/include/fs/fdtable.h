#ifndef _FD_TABLE_H
#define _FD_TABLE_H

#include <sync/spinlock.h>

struct file;

struct file_table {
	struct file **files;
	size_t count;
	size_t capacity;
	size_t next_fd;
	spinlock_t lock;
};

struct file_table* file_table_create(size_t capacity);
struct file_table *file_table_clone(struct file_table *table);
void file_table_destroy(struct file_table* table);

int file_table_add_file(struct file_table* table, struct file* file);
int file_table_remove_file(struct file_table* table, int fd);

#endif