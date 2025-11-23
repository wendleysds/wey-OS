#include <stdint.h>

struct file;

struct file_operations {
    int (*read)(struct file *file, void *buffer, uint64_t count);
    int (*write)(struct file *file, const void *buffer, uint64_t count);
    int (*lseek)(struct file *file, uint64_t offset, uint8_t whence);
    int (*close)(struct file *file);
};

struct file {
	uint64_t pos;
	uint64_t size;

	const struct file_operations* ops;

	void* private;
};
