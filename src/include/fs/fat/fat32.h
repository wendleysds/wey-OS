#ifndef _FAT32_H
#define _FAT32_H

#include <fs/fat/fatdefs.h>
#include <fs/file.h>
#include <stdint.h>

int FAT32_init(struct FAT* fat);

struct FATFileDescriptor* FAT32_open(const char* pathname, uint8_t flags, uint8_t mode);
int FAT32_stat(const char* pathname, struct Stat* statbuf);
int FAT32_seek(struct FATFileDescriptor* ffd, uint32_t offset, uint8_t whence);
int FAT32_read(struct FATFileDescriptor* ffd, void* buffer, uint32_t count);
int FAT32_write(struct FATFileDescriptor* ffd, const void* buffer, uint32_t size);
int FAT32_close(struct FATFileDescriptor* ffd);

#endif
