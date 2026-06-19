#ifndef _CPIO_H
#define _CPIO_H

#include <def/compile.h>
#include <fs/vfs.h>

struct cpio_header{
	char magic[6];  // "070701"
	char ino[8];
	char mode[8];
	char uid[8];
	char gid[8];
	char nlink[8];
	char mtime[8];
	char filesize[8];
	char devmajor[8];
	char devminor[8];
	char rdevmajor[8];
	char rdevminor[8];
	char namesize[8];
	char check[8];
} __packed;

struct cpio_file_iter {
	uint32_t ino;
	struct qstr name;
	uintptr_t content_ptr;
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

uint32_t cpio_parse_field(const char field[8]);
int cpio_initramfs_iterate(void* initrd_start, size_t size, void** cursor, struct cpio_file_iter* file_buffer);

#endif