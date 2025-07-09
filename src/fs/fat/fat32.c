#include "def/config.h"
#include "fs/fs.h"
#include <fs/fat/fat32.h>
#include <fs/fat/fatdefs.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <lib/string.h>
#include <lib/utils.h>
#include <def/status.h>
#include <drivers/terminal.h>
#include <io/stream.h>
#include <stdint.h>

/*
 * Main module for FAT32 parse and handler
 * This module provides functions to initialize the FAT32 filesystem,
 * open files, read/write data, and manage directories.
 */

#define CLUSTER_SIZE 4096
#define EOF 0x0FFFFFF8
#define CHK_EOF(cluster) \
	(cluster >= EOF)

#define _SEC(lba) \
	(lba * 0x200)

static uint32_t _cluster_to_lba(struct FAT* fat, uint32_t cluster){
	return fat->firstDataSector + ((cluster - 2) * fat->headers.boot.secPerClus);
}

static uint32_t _get_cluster_entry(struct FAT32DirectoryEntry* entry){
	return (entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
}

static uint32_t _next_cluster(struct FAT* fat, uint32_t current){
	return fat->table[current] & 0x0FFFFFFF; // Mask to get the cluster number
}

static uint32_t _find_free_cluster(struct FAT* fat){
	if(!fat || !fat->table){
		return 0xFFFFFFFF;
	}

	uint32_t cluster;
	if(fat->fsInfo.nextFreeCluster >= fat->headers.extended.rootClus && fat->fsInfo.nextFreeCluster < fat->totalClusters){
		cluster = fat->fsInfo.nextFreeCluster;
		for (; cluster < fat->totalClusters; cluster++)
		{
			if(_next_cluster(fat, cluster) == FAT_UNUSED){
				return cluster;
			}
		}
	}

	cluster = fat->headers.extended.rootClus;
	for (; cluster < fat->totalClusters; cluster++)
	{
		if(_next_cluster(fat, cluster) == FAT_UNUSED){
			return cluster;
		}
	}

	return 0xFFFFFFFF;
}

static uint32_t _reserve_next_cluster(struct FAT* fat){
	uint32_t cluster = _find_free_cluster(fat);
	if(cluster == 0xFFFFFFFF){
		return cluster;
	}

	fat->fsInfo.nextFreeCluster = cluster + 1;
	if (fat->fsInfo.freeClusterCount != 0xFFFFFFFF) {
		fat->fsInfo.freeClusterCount--;
	}

	return cluster;
}

static int _free_chain(struct FAT* fat, uint32_t start){
	if(!fat || start < 2 || start > fat->totalClusters){
		return INVALID_ARG;
	}

	int count = 0;
	uint32_t cur = start;
	while(1){
		if(!(cur <= 0 < fat->totalClusters)){
			break;
		}

		uint32_t next = _next_cluster(fat, cur);
		fat->table[cur] = FAT_UNUSED;
		count++;

		if(CHK_EOF(next)){
			break;
		}

		cur = next;
		
		if(count > fat->totalClusters){
			break;
		}
	}

	if(fat->fsInfo.freeClusterCount != 0xFFFFFFFF){
		fat->fsInfo.freeClusterCount += count;
	}

	fat->fsInfo.nextFreeCluster = start;

	return SUCCESS;
}

static int _get_entry(struct FAT* fat, char* itemName, uint32_t dirCluster, struct FAT32DirectoryEntry* entrybuff){
	if(!fat || !itemName || dirCluster < 2 || dirCluster > fat->totalClusters || !entrybuff){
		return INVALID_ARG;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	char filename[12];
	struct FAT32DirectoryEntry buffer;
	uint32_t lba = _cluster_to_lba(fat, dirCluster);

	stream_seek(stream, lba);
	_format_fat_name(itemName, filename);
	while(1){
		if(stream_read(stream, &buffer, sizeof(buffer)) != SUCCESS){
			stream_dispose(stream);
			return ERROR_IO;
		}

		if(buffer.DIR_Name[0] == 0x0){
			stream_dispose(stream);
			return FILE_NOT_FOUND;
		}

		if(buffer.DIR_Name[0] == 0xE5){
			continue;
		}

		if (strncmp((char *)buffer.DIR_Name, filename, 11) == 0){
			memcpy(entrybuff, &buffer, sizeof(struct FAT32DirectoryEntry));
			stream_dispose(stream);
			return SUCCESS;
		}
	}
}

static int _traverse_path(struct FAT* fat, const char* path, int* outDirCluster, struct FAT32DirectoryEntry* entrybuff){
	if(!fat || !entrybuff || !path || strlen(path) > PATH_MAX){
		return INVALID_ARG;
	}

	char pathCopy[PATH_MAX];
	strncpy(pathCopy, path, PATH_MAX - 1);

	char *token = strtok(pathCopy, "/");
	if (!token){
		return INVALID_ARG;
	}

	*outDirCluster = fat->headers.extended.rootClus;
	int status = _get_entry(fat, token, *outDirCluster, entrybuff);
	if(status != SUCCESS){
		return status;
	}

	while((token = strtok(NULL, "/"))){
		if(entrybuff->DIR_Attr & ATTR_ARCHIVE){
			return NOT_SUPPORTED;
		}

		*outDirCluster = _get_cluster_entry(&entrybuff);
		status = _get_entry(fat, token, *outDirCluster, entrybuff);
		if(status != SUCCESS){
			return status;
		}
	}

	return status;
}

static int _update_fat(struct FAT* fat){
	if(!fat){
		return INVALID_ARG;
	}

	struct Stream* stream = stream_new();
	if(!stream) return NO_MEMORY;

	uint32_t fatStartSector = fat->headers.boot.rsvdSecCnt;
	uint32_t fatBystes = fat->headers.extended.FATSz32 * fat->headers.boot.bytesPerSec;

	stream_seek(stream, fat->headers.extended.FSInfo);
	int status = stream_write(stream, &fat->fsInfo, sizeof(fat->fsInfo));
	if(status != SUCCESS) goto err;

	stream_seek(stream, fatStartSector);
	int status = stream_write(stream, &fat->table, fatStartSector);
	if(status != SUCCESS) goto err;

err:
	stream_dispose(stream);
	return status;
}

static int _creaty_entry(struct FAT* fat, const char* pathname, int attr){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	char pathCopy[PATH_MAX];
	strncpy(pathCopy, pathname, PATH_MAX - 1);

	char *pathpart = strrchr(pathCopy, '/');
	char *filename = pathpart + 1;
	*pathpart = '\0';

	if (strlen(filename) > 11)
		return NOT_SUPPORTED;

	struct FAT32DirectoryEntry buffer;
	memset(&buffer, 0, sizeof(buffer));
}

static int _remove_entry(struct FAT* fat, const char* pathname){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	const char *filename = strrchr(pathname, '/');
	if (!filename || strlen(filename + 1) > 11) {
		return NOT_SUPPORTED;
	}

	struct FAT32DirectoryEntry entry;
	int dirCluster;
	int status = _traverse_path(fat, pathname, &dirCluster, &entry);
	if (status != SUCCESS) {
		return status;
	}

	if (entry.DIR_Attr & ATTR_DIRECTORY) {
		return OP_ABORTED;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	uint32_t lba = _cluster_to_lba(fat, dirCluster);
	stream_seek(stream, lba);

	struct FAT32DirectoryEntry buffer;
	while(1){
		if(stream_read(stream, &buffer, sizeof(buffer)) != SUCCESS){
			stream_dispose(stream);
			return ERROR_IO;
		}

		if(memcmp(buffer.DIR_Name, entry.DIR_Name, 11) == 0){
			buffer.DIR_Name[0] = 0xE5;
			uint32_t cluster = _get_cluster_entry(&buffer);
			_free_chain(fat, cluster);
			fat->fsInfo.nextFreeCluster = cluster;

			stream_seek(stream, stream->cursor - sizeof(buffer)); // Go back
			if(stream_write(stream, &buffer, sizeof(buffer)) != SUCCESS){
				stream_dispose(stream);
				return ERROR_IO;
			}
			break;
		}
	}

	stream_dispose(stream);
	return _update_fat(fat);
}

static int _truncate_entry(struct FAT* fat, struct FAT32DirectoryEntry* entry){
	if(!fat || !entry){
		return INVALID_ARG;
	}

	uint32_t cluster = _get_cluster_entry(entry);
	_free_chain(fat, cluster);
	fat->table[cluster] = EOF;
	entry->DIR_FileSize = 0;

	return SUCCESS;
}

int FAT32_init(struct FAT* fat){
	if(!fat){
		return INVALID_ARG;
	}

	memset(fat, 0x0, sizeof(struct FAT));

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	int status = SUCCESS;

	stream_seek(stream, 0);
	if(stream_read(stream, &fat->headers, sizeof(fat->headers)) != SUCCESS){
		status = ERROR_IO;
		goto err; // Failed to read the FAT32 headers
	}

	stream_seek(stream, 1); 
	if(stream_read(stream, &fat->fsInfo, sizeof(fat->fsInfo)) != SUCCESS){
		status = ERROR_IO;
		goto err; // Failed to read the FSInfo structure
	}

	if(fat->fsInfo.leadSignature != 0x41615252 || 
	   fat->fsInfo.structSignature != 0x61417272 || 
	   fat->fsInfo.trailSignature != 0xAA550000){
		status = ERROR_IO;
		goto err; // Invalid FSInfo structure
	}

	uint32_t fatStartSector = fat->headers.boot.rsvdSecCnt;
	uint32_t fatSize = fat->headers.extended.FATSz32;
	uint32_t fatTotalClusters = fatSize * fat->headers.boot.bytesPerSec / 4;
	uint32_t fatBystes = fatSize * fat->headers.boot.bytesPerSec;

	uint32_t* fat_table = (uint32_t*)kmalloc(fatBystes);
	if(!fat_table){
		status = NO_MEMORY;
		goto err; // Failed to allocate memory for FAT table
	}

	stream_seek(stream, fatStartSector);
	if(stream_read(stream, fat_table, fatBystes) != SUCCESS){
		kfree(fat_table);
		status = ERROR_IO;
		goto err; // Failed to read the FAT table
	}

	fat->firstDataSector = fatStartSector + (fat->headers.boot.numFATs * fatSize);
	fat->totalClusters = fatTotalClusters;
	fat->table = fat_table;
	fat->rootClus = fat->headers.extended.rootClus;

err:
	stream_dispose(stream);
	return status;
}

int FAT32_open(struct FAT* fat, void** outFDPtr, const char *pathname, uint8_t flags){
	if(!fat || !outFDPtr || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	int outDirCluster;
	struct FAT32DirectoryEntry buffer;

	int status = _traverse_path(fat, pathname, &outDirCluster, &buffer);
	if(status == FILE_NOT_FOUND && (flags & O_CREAT)){
		status = _creaty_entry(fat, pathname, ATTR_ARCHIVE);
		if(status != SUCCESS){
			return status;
		}
		status = _traverse_path(fat, pathname, &outDirCluster, &buffer);
	}
	if(status != SUCCESS){
		return status;
	}

	if(flags & O_TRUNC){
		status = _truncate_entry(fat, &buffer);
		if(status != SUCCESS){
			return status;
		}
	}

	struct FATFileDescriptor* ffd = (struct FATFileDescriptor*)kcalloc(sizeof(struct FATFileDescriptor));
	if(!ffd){
		return NO_MEMORY;
	}

	ffd->firtCluster = _get_cluster_entry(&buffer);
	ffd->currentCluster = ffd->firtCluster;
	ffd->dirCluster = outDirCluster;
	ffd->entry = buffer;
	*outFDPtr = (void*)ffd;

	return SUCCESS;
}

int FAT32_stat(struct FAT* fat, const char* restrict pathname, struct Stat* restrict statbuf){
	if(!fat || !statbuf || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}
}

int FAT32_read(struct FAT* fat, struct FATFileDescriptor *ffd, void *buffer, uint32_t count){
	if(!fat || !ffd || !buffer || count < 0){
		return INVALID_ARG;
	}
}

int FAT32_seek(struct FAT* fat, struct FATFileDescriptor* ffd, uint32_t offset, uint8_t whence){
	if(!fat || !ffd){
		return INVALID_ARG;
	}
}

int FAT32_write(struct FAT *fat, struct FATFileDescriptor *ffd, const void *buffer, uint32_t size){
	if(!fat || !ffd || !buffer || size < 0){
		return INVALID_ARG;
	}
}

int FAT32_close(struct FATFileDescriptor *ffd){
	if(!ffd){
		return INVALID_ARG;
	}

	kfree(ffd);

	return SUCCESS;
}
