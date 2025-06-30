#include "def/config.h"
#include <fs/fat/fat32.h>
#include <fs/fat/fatdefs.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <lib/string.h>
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

static int firstDataSector = -1; 
static struct FATHeaders headers;

static uint32_t _cluster_to_lba(uint32_t cluster){
	return firstDataSector + ((cluster - 2) * headers.boot.secPerClus);
}

static uint32_t _get_cluster_entry(struct FAT32DirectoryEntry* entry){
	return (entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
}

static uint32_t _next_cluster(struct FAT* fat, uint32_t current){
	uint32_t next = fat->table[current];
	return next & 0x0FFFFFFF; // Mask to get the cluster number
}

static void strToUpper(char* str){
	for(int i = 0; str[i]; i++){
		if(str[i] >= 'a' && str[i] <= 'z'){
			str[i] -= 32; // Convert to uppercase
		}
	}
}

// Format a filename to fit in FAT32 8.3 format
static void _format_fat_name(const char* filename, char* out){
	memset(out, ' ', 11);

	if(!filename || strlen(filename) == 0){
		out[0] = '\0';
		return;
	}

	if(strlen(filename) > 11){
		// If the filename is longer than 11 characters, truncate it
		strncpy(out, filename, 8);
		
		// Copy extension (last 3 chars after dot, if present)
		const char* dot = strrchr(filename, '.');
		if (dot && *(dot + 1)) {
			const char* ext = dot + 1;
			size_t extLen = strlen(ext);
			if (extLen > 3) ext += extLen - 3;
			strcpy(out + 8, ext);
		} else {
			memset(out + 8, ' ', 3);
		}
		
		out[6] = '~'; // Indicate that the name is truncated
		out[7] = '1'; // Add a number to differentiate
		out[11] = '\0';

		strToUpper(out);

		return;
	}

	const char* dot = strrchr(filename, '.');
	uint8_t nameLenght = 0;
	uint8_t extLenght = 0;

	for(const char* p = filename; *p && p != dot && nameLenght < 8; p++){
		out[nameLenght++] = (unsigned char)*p;
	}

	if(dot && *(dot + 1)){
		const char* ext = dot + 1;
		while(*ext && extLenght < 3){
			out[8 + extLenght++] = (unsigned char)*ext;
			ext++;
		}
	}

	out[11] = '\0';

	strToUpper(out);
}

static struct FAT32DirectoryEntry* _copy_fat_entry(struct FAT32DirectoryEntry* entry){
	struct FAT32DirectoryEntry* fileEntry = (struct FAT32DirectoryEntry*)kmalloc(sizeof(struct FAT32DirectoryEntry));
	if(!fileEntry){
		return 0x0;
	}

	memcpy(fileEntry, entry, sizeof(struct FAT32DirectoryEntry));
	return fileEntry;
}

static int _get_directory_itens_count(struct Directory* dir){
	uint32_t dirSector = _cluster_to_lba(dir->firstCluster) * headers.boot.bytesPerSec;

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	struct FAT32DirectoryEntry entry;
	stream_seek(stream, dirSector);
	int count = 0;

	do{
		if(stream_read(stream, &entry, sizeof(entry)) != SUCCESS){
			stream_dispose(stream);
			return ERROR_IO;
		}

		if(entry.DIR_Name[0] == 0x0) break;
		if(entry.DIR_Name[0] == 0xE5) continue;
		if(entry.DIR_Attr & (ATTR_LONG_NAME)) continue; // Long file name entry mask

		count++;

	} while(1);

	stream_dispose(stream);

	return count;
}

// Transform a FAT32 directory entry into a Directory structure
static struct Directory* _create_directory_entry(struct FAT32DirectoryEntry* entry){
	if(!(entry->DIR_Attr & ATTR_DIRECTORY)){
		return 0x0;
	}

	struct Directory* dir = (struct Directory*)kcalloc(sizeof(struct Directory));
	if(!dir){
		return 0x0;
	}

	dir->entry = entry;
	dir->firstCluster = _get_cluster_entry(entry);
	dir->currentCluster = dir->firstCluster;

	dir->itensCount = _get_directory_itens_count(dir);
	if(dir->itensCount < 0){
		kfree(dir);
		return 0x0;
	}

	return dir;
}

static int _get_item_in_diretory(char* itemName, struct FATItem* itembuff, struct Directory* dir){
	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	struct FAT32DirectoryEntry buffer;
	char filename[12];
	_format_fat_name(itemName, filename);

	uint32_t lba = _cluster_to_lba(dir->firstCluster);
	stream_seek(stream, lba* headers.boot.bytesPerSec);

	int itemCount = dir->itensCount;
	while(1){
		if(stream_read(stream, &buffer, sizeof(buffer)) != SUCCESS){
			stream_dispose(stream);
			return ERROR_IO;
		}

		if(buffer.DIR_Name[0] == 0x0){
			stream_dispose(stream);
			return FILE_NOT_FOUND; // Reached the end of the directory
		}

		if(buffer.DIR_Name[0] == 0xE5){
			continue;
		}

		if(buffer.DIR_Attr & (ATTR_LONG_NAME)){
			continue;
		}

		if(itemCount-- <= 0){
			stream_dispose(stream);
			return FILE_NOT_FOUND; // No more items in the directory
		}

		if(strncmp((char*)buffer.DIR_Name, filename, 11) == 0){
			struct FAT32DirectoryEntry* entry = _copy_fat_entry(&buffer);
			if(!entry){
				stream_dispose(stream);
				return NO_MEMORY;
			}

			enum ItemType type = (buffer.DIR_Attr & 0x10) ? Directory : File;
			itembuff->type = type;
			itembuff->file = entry;
			itembuff->offsetInBytes = stream->unused - sizeof(struct FAT32DirectoryEntry); // Offset in the stream where the entry was found

			stream_dispose(stream);

			if(type == Directory){
				struct Directory* dir = _create_directory_entry(entry);
				if(!dir){
					return NO_MEMORY;
				}

				itembuff->directory = dir;
			}

			return SUCCESS;
		}
	}
}

static int _traverse_path(struct FAT* fat, const char* path, struct FATItem* itembuff){
	if(!fat || !path || strlen(path) == 0){
		return INVALID_ARG;
	}

	char pathCopy[PATH_MAX];
	memset(pathCopy, 0x0, sizeof(pathCopy));
	strncpy(pathCopy, path, PATH_MAX - 1);

	char* token = strtok(pathCopy, "/");
	if(!token){
		return INVALID_ARG;
	}

	// Set the root directory as the starting point
	if(_get_item_in_diretory(token, itembuff, &fat->rootDir) != SUCCESS){
		return FILE_NOT_FOUND;
	}

	while((token = strtok(0x0, "/"))){
		if(itembuff->type != Directory){
			return NOT_SUPPORTED; // Cannot traverse into a file
		}
		if(_get_item_in_diretory(token, itembuff, itembuff->directory) != SUCCESS){
			return FILE_NOT_FOUND;
		}
	}

	return SUCCESS;
}

static int _get_root_directory(struct FAT* fat){
	fat->rootDir.firstCluster = headers.extended.rootClus;
	fat->rootDir.currentCluster = headers.extended.rootClus;

	uint16_t itemCount = _get_directory_itens_count(&fat->rootDir);
	if(itemCount < 0)
		return itemCount;

	fat->rootDir.entry = 0x0; // Root don´t have entry becuse is the ROOT
	fat->rootDir.itensCount = itemCount;

	return SUCCESS;
}

int FAT32_init(struct FAT* fat){
	if(!fat){
		return INVALID_ARG;
	}

	memset(fat, 0x0, sizeof(struct FAT));

	fat->readStream = stream_new();
	if(!fat->readStream){
		return NO_MEMORY;
	}

	fat->writeStream = stream_new();
	if(!fat->writeStream){
		stream_dispose(fat->readStream);
		return NO_MEMORY;
	}

	fat->clusterReadStream = stream_new();
	if(!fat->clusterReadStream){
		stream_dispose(fat->readStream);
		stream_dispose(fat->writeStream);
		return NO_MEMORY;
	}

	stream_seek(fat->readStream, 0);
	if(stream_read(fat->readStream, &fat->headers, sizeof(fat->headers)) != SUCCESS){
		goto fail; // Failed to read the FAT32 headers
	}

	stream_seek(fat->readStream, 512); 
	if(stream_read(fat->readStream, &fat->fsInfo, sizeof(fat->fsInfo)) != SUCCESS){
		goto fail; // Failed to read the FSInfo structure
	}

	if(fat->fsInfo.leadSignature != 0x41615252 || 
	   fat->fsInfo.structSignature != 0x61417272 || 
	   fat->fsInfo.trailSignature != 0xAA550000){
		goto fail; // Invalid FSInfo structure
	}

	uint32_t fatStartSector = fat->headers.boot.rsvdSecCnt;
	uint32_t fatSize = fat->headers.extended.FATSz32;
	uint32_t fatTotalClusters = fatSize * fat->headers.boot.bytesPerSec / 4;
	uint32_t fatBystes = fatSize * fat->headers.boot.bytesPerSec;

	uint32_t* fat_table = (uint32_t*)kmalloc(fatBystes);
	if(!fat_table){
		goto fail; // Failed to allocate memory for FAT table
	}

	stream_seek(fat->readStream, fatStartSector * fat->headers.boot.bytesPerSec);
	if(stream_read(fat->readStream, fat_table, fatBystes) != SUCCESS){
		kfree(fat_table);
		goto fail; // Failed to read the FAT table
	}

	fat->firstDataSector = fatStartSector + (fat->headers.boot.numFATs * fatSize);
	fat->totalClusters = fatTotalClusters;
	fat->table = fat_table;

	headers = fat->headers;
	firstDataSector = fat->firstDataSector;

	if(_get_root_directory(fat) != SUCCESS){
		kfree(fat_table);
		goto fail; // Failed to get the root directory
	}

	return SUCCESS;

fail:
	stream_dispose(fat->readStream);
	stream_dispose(fat->writeStream);
	stream_dispose(fat->clusterReadStream);
	return FAILED; // Failed to initialize FAT32 filesystem
}

struct FATFileDescriptor* FAT32_open(struct FAT* fat, const char *pathname, uint8_t flags){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return 0x0;
	}

	struct FATFileDescriptor* fd = kmalloc(sizeof(struct FATFileDescriptor));
	if(!fd){
		return 0x0;
	}

	struct FATItem itembuff;

	int status = _traverse_path(fat, pathname, &itembuff);
	if(status != SUCCESS){
		kfree(fd);
		return 0x0;
	}

	struct FATItem* item = (struct FATItem*)kmalloc(sizeof(struct FATItem));
	if(!item){
		kfree(fd);
		return 0x0;
	}

	memcpy(item, &itembuff, sizeof(struct FATItem));

	uint32_t cluster = 0;
	if(item->type == Directory)
		cluster = _get_cluster_entry(item->directory->entry);
	else if(item->type == File)
		cluster = _get_cluster_entry(item->file);

	fd->item = item;
	fd->firstCluster = cluster;
	fd->currentCluster = cluster;
	fd->cursor = 0;

	return fd;
}

int FAT32_stat(struct FAT* fat, const char* restrict pathname, struct Stat* restrict statbuf){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX || !statbuf){
		return INVALID_ARG;
	}

	struct FATItem itembuff;

	int status = _traverse_path(fat, pathname, &itembuff);
	if(status != SUCCESS){
		return status;
	}

	struct FAT32DirectoryEntry* entry;
	if(itembuff.type == Directory){
		entry = itembuff.directory->entry;
	} else if (itembuff.type == File){
		entry = itembuff.file;
	} else{
		return NOT_SUPPORTED;
	}

	statbuf->fileSize = entry->DIR_FileSize;
	statbuf->attr = entry->DIR_Attr;
	statbuf->creDate = entry->DIR_CrtDate;
	statbuf->modDate = entry->DIR_WrtTime;

	return SUCCESS;
}

int FAT32_write(struct FAT *fat, struct FATFileDescriptor *ffd, const void *buffer, uint32_t size){
	if(!fat|| !ffd || !buffer){
		return INVALID_ARG;
	}

	if(ffd->item->type != File){
		return NOT_SUPPORTED; // Writing is only supported for files
	}

	uint32_t cursor = ffd->cursor;
		
	uint32_t sector = _cluster_to_lba(ffd->currentCluster);
	uint32_t offset = sector * headers.boot.bytesPerSec;

	uint32_t remaining = size;
	uint32_t totalWritten = 0;

	while(remaining > 0){
		uint32_t clusterOffset = cursor % CLUSTER_SIZE;
		uint32_t bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;
		uint32_t toWrite = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;

		stream_seek(fat->writeStream, offset + clusterOffset);
		if(stream_write(fat->writeStream, (uint8_t*)buffer + totalWritten, toWrite) != SUCCESS){
			return ERROR_IO;
		}

		remaining -= toWrite;
		cursor += toWrite;
		totalWritten += toWrite;

		if(cursor % CLUSTER_SIZE == 0){
			uint32_t next = _next_cluster(fat, ffd->currentCluster);
			if(next >= 0x0FFFFFF8){ // End of file
				break;
			}

			ffd->currentCluster = next;
			sector = _cluster_to_lba(ffd->currentCluster);
			offset = sector * headers.boot.bytesPerSec;
			
			stream_flush(fat->writeStream);
		}
	}

	stream_flush(fat->writeStream);

	ffd->item->file->DIR_FileSize += size;
	ffd->cursor += size;

	// update the file entry in the directory

	return totalWritten;
}

int FAT32_read(struct FAT* fat, struct FATFileDescriptor *ffd, void *buffer, uint32_t count){
	if (!fat || !ffd || !ffd->item || ffd->item->type != File || !buffer)
		return INVALID_ARG;

	uint32_t cursor = ffd->cursor;
	uint32_t fileSize = ffd->item->file->DIR_FileSize;

	if (cursor >= fileSize) return READ_FAIL; // Cannot read beyond the end of the file
	if ((cursor + count) > fileSize)
		count = fileSize - cursor;

	uint32_t remaining = count;
	uint32_t totalRead = 0;

	uint32_t sector = _cluster_to_lba(ffd->currentCluster);
	uint32_t offset = sector * headers.boot.bytesPerSec;

	while (remaining > 0) {
		uint32_t clusterOffset = cursor % CLUSTER_SIZE;
		uint32_t bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;
		uint32_t toRead = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;

		stream_seek(fat->readStream, offset + clusterOffset);
		if(stream_read(fat->readStream, (uint8_t*)buffer + totalRead, toRead) != SUCCESS){
			return ERROR_IO;
		}

		cursor += toRead;
		totalRead += toRead;
		remaining -= toRead;

		if (cursor % CLUSTER_SIZE == 0) {
			uint32_t next = _next_cluster(fat, ffd->currentCluster);
			if (next >= 0x0FFFFFF8)
				break;

			ffd->currentCluster = next;
			sector = _cluster_to_lba(ffd->currentCluster);
			offset = sector * headers.boot.bytesPerSec;
		}
	}

	ffd->cursor = cursor;
	return totalRead;
}

int FAT32_seek(struct FAT* fat, struct FATFileDescriptor* ffd, uint32_t offset, uint8_t whence){
	if(!ffd || !ffd->item || ffd->item->type != File){
		return INVALID_ARG;
	}

	uint32_t target;
	switch(whence){
		case SEEK_SET:
			target = offset	;	
			break;
		case SEEK_CUR:
			target = ffd->cursor + offset;
			break;
		case SEEK_END:
			if(offset > ffd->item->file->DIR_FileSize){
				return INVALID_ARG; // Cannot seek beyond file size
			}

			target = ffd->item->file->DIR_FileSize - offset;
			break;
		default:
			return INVALID_ARG;
	}

	if(target > ffd->item->file->DIR_FileSize){
		return INVALID_ARG; // Cannot seek beyond file size
	}

	uint32_t clusterOffset = target / CLUSTER_SIZE;

	uint32_t cluster = ffd->firstCluster;
	for(uint32_t i = 0; i < clusterOffset; i++){
		cluster= _next_cluster(fat, cluster);
		if(cluster >= 0x0FFFFFF8) {
			return END_OF_FILE; // Reached end of file
		}
	}

	ffd->currentCluster = cluster;
	ffd->cursor = target;

	return SUCCESS;
}

int FAT32_update_file(struct FAT* fat, struct FATFileDescriptor* ffd){
	struct FATItem* item = ffd->item;
	if(!item || !fat){
		return INVALID_ARG;
	}

	struct FAT32DirectoryEntry* entry;

	if(item->type == Directory){
		if(!item->directory || !item->directory->entry){
			return NULL_PTR; // Directory entry is not set
		}
		
		entry = item->directory->entry;
	} else if(item->type == File){
		entry = item->file;
	} else {
		return NOT_SUPPORTED; // Only files and directories can be updated
	}

	if(!entry){
		return NULL_PTR;
	}

	stream_seek(fat->writeStream, item->offsetInBytes);
	if(stream_write(fat->writeStream, entry, sizeof(struct FAT32DirectoryEntry)) != SUCCESS){
		return ERROR_IO;
	}

	stream_flush(fat->writeStream);

	return SUCCESS;
}

int FAT32_close(struct FATFileDescriptor *ffd){
	if(!ffd)
		return INVALID_ARG;

	if(ffd->item->type == Directory){
		kfree(ffd->item->directory->entry);
		kfree(ffd->item->directory);
	} else if(ffd->item->type == File){
		kfree(ffd->item->file);
	} else {
		kfree(ffd->item->fileLong);
	}

	kfree(ffd->item);
	kfree(ffd);

	return SUCCESS;
}
