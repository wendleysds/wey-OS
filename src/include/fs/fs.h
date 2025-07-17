#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include <fs/file.h>
#include <stdint.h>

#define O_RDONLY    0x1
#define O_WRONLY    0x2
#define O_RDWR      0x3
#define O_CREAT     0x4
#define O_TRUNC     0x8
#define O_DIRECTORY 0x10

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct FileDescriptor {
	uint8_t index;
	uint8_t flags;
	void* descriptorPtr;
};

void fs_init();

int open(const char *pathname, int flags);
int stat(const char* restrict pathname, struct stat* restrict statbuf);
int read(int fd, void* buffer, uint32_t count);
int write(int fd, const void* buffer, uint32_t count);
int lseek(int fd, int offset, int whence);
int close(int fd);

#endif

