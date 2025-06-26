#include "drivers/terminal.h"
#include "fs/fat/fatdefs.h"
#include "memory/kheap.h"
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fat/fat32.h>
#include <lib/mem.h>
#include <lib/string.h>
#include <core/kernel.h>
#include <def/status.h>
#include <def/config.h>
#include <stdint.h>

static struct FileDescriptor* fileDescriptors[FILE_DESCRIPTORS_MAX];
static struct FAT fat;

void fs_init() {
	memset(fileDescriptors, 0x0, sizeof(fileDescriptors));

	int status = FAT32_init(&fat); 
	if(status != SUCCESS){
		panic("Failed to initialize File System! STATUS %d", status);
	}
}

static uint8_t _get_next_fd_index(){
	uint8_t i = 3; // First three indices are reserved for stdin, stdout, and stderr
	for (; i < FILE_DESCRIPTORS_MAX - 3; i++) {
		if(fileDescriptors[i] == 0x0)
			return i;
	}

	return ERROR;
}

int open(const char *pathname, int flags){
	int p = _get_next_fd_index();
	if(p == ERROR)
		return p;

	void* ffd = FAT32_open(&fat, pathname, flags);
	if(!ffd){
		return FILE_NOT_FOUND;
	}

	struct FileDescriptor* fd = (struct FileDescriptor*)kmalloc(sizeof(struct FileDescriptor));
	if(!fd){
		FAT32_close(ffd);
		return NO_MEMORY;
	}

	fd->descriptorPtr = ffd;
	fd->flags = flags;
	fd->index = p;
	fileDescriptors[p] = fd;

	return p;
}

int stat(const char* restrict pathname, struct Stat* restrict statbuf){
	return FAT32_stat(&fat, pathname, statbuf);
}

int read(int fd, void *buffer, uint32_t count){
	if(fd < 0 || !buffer){
		return INVALID_ARG;
	}

	struct FileDescriptor* fdPtr = fileDescriptors[fd];
	if(!fdPtr){
		return NULL_PTR;
	}

	if(fdPtr->flags & O_RDONLY)
		return FAT32_read(&fat, fdPtr->descriptorPtr, buffer, count);
	else
		return READ_FAIL;
}

int write(int fd, const void *buffer, uint32_t count){
	if(fd < 0 || !buffer){
		return INVALID_ARG;
	}

	if(fd == 0){
		return WRITE_FAIL; // Cannot write to stdin
	}

	struct FileDescriptor* fdPtr = fileDescriptors[fd];
	if(!fdPtr){
		return NULL_PTR;
	}

	if(fdPtr->flags & O_WRONLY)
		return FAT32_write(&fat, fdPtr->descriptorPtr, buffer, count);
	else
		return WRITE_FAIL;
}

int lseek(int fd, int offset, int whence){
	if(fd < 0 || offset < 0){
		return INVALID_ARG;
	}

	struct FileDescriptor* fdPtr = fileDescriptors[fd];
	if(!fdPtr){
		return NULL_PTR;
	}

	return FAT32_seek(&fat, fdPtr->descriptorPtr, offset, whence);
}

int close(int fd){
	if(fd <= 3 || fd >= FILE_DESCRIPTORS_MAX){
		return INVALID_ARG; // Reserved file descriptors cannot be closed
	}

	struct FileDescriptor* fdPtr = fileDescriptors[fd];
	if(!fdPtr)
		return ERROR;

	FAT32_close(fdPtr->descriptorPtr);
	kfree(fdPtr);
	fileDescriptors[fd] = 0x0;

	return 0;
}
