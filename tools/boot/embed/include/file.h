#include <stdint.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct file;

typedef int32_t off_t;

struct file_operations {
    int (*read)(struct file *file, void *buffer, uint64_t count);
    int (*lseek)(struct file *file, off_t offset, uint8_t whence);
    int (*close)(struct file *file);
};

struct file {
	uint32_t pos;
	uint32_t size;

	const struct file_operations* ops;

	void* private;
};
