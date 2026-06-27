#ifndef _CPIO_H
#define _CPIO_H

#include <def/compile.h>
#include <fs/vfs.h>

struct cpio_file_iter {
	uint32_t ino;
	const char* name;
	const uint8_t* content_ptr;
	uint32_t filesize;

	uint32_t mode;
	uint32_t uid, gid;
	uint32_t nlink;
	uint32_t mtime;
	uint32_t devmajor;
	uint32_t devminor;
	uint32_t rdevmajor;
	uint32_t rdevminor;
	uint32_t check;
};

int cpio_initramfs_iterate(void* initrd_start, size_t size, const uint8_t** cursor, struct cpio_file_iter* file_buffer);

#endif