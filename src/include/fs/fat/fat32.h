#ifndef _FAT32_H
#define _FAT32_H

#include <fs/fat/fatdefs.h>
#include <fs/file.h>
#include <stdint.h>

int FAT32_init(struct FAT* fat);

int FAT32_open(struct FAT* fat, void** outFDPtr, const char* pathname, uint8_t flags);
int FAT32_stat(struct FAT* fat, const char* restrict pathname, struct stat* restrict statbuf);
int FAT32_seek(struct FAT* fat, struct FATFileDescriptor* ffd, uint32_t offset, uint8_t whence);
int FAT32_read(struct FAT* fat, struct FATFileDescriptor* ffd, void* buffer, uint32_t count);
int FAT32_write(struct FAT* fat, struct FATFileDescriptor* ffd, const void* buffer, uint32_t size);
int FAT32_close(struct FATFileDescriptor* ffd);

int FAT32_mkdir(struct FAT* fat, const char* pathname);
int FAT32_rmdir(struct FAT* fat, const char* pathname);
int FAT32_create(struct FAT* fat, const char* pathname);
int FAT32_remove(struct FAT* fat, const char* pathname);

#endif
