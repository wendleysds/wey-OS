#ifndef _FAT32_H
#define _FAT32_H

#include <fs/fat/fatdefs.h>
#include <fs/fs.h>
#include <stdint.h>

int FAT32_init(struct FAT* fat);

struct FATFileDescriptor* FAT32_open(struct FAT* fat, const char* pathname, uint8_t flags);
int FAT32_stat(struct FAT* fat, const char* restrict pathname, struct Stat* restrict statbuf);
int FAT32_seek(struct FAT* fat, struct FATFileDescriptor* ffd, uint32_t offset, uint8_t whence);
int FAT32_read(struct FAT* fat, struct FATFileDescriptor* ffd, void* buffer, uint32_t count);
int FAT32_write(struct FAT* fat, struct FATFileDescriptor* ffd, const void* buffer, uint32_t size);
int FAT32_update_file(struct FAT* fat, struct FATFileDescriptor* ffd);
int FAT32_close(struct FATFileDescriptor* ffd);

#endif
