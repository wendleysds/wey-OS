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

static int firstDataSector = -1; 
static struct FATHeaders headers;

static uint32_t _cluster_to_lba(uint32_t cluster){
	return firstDataSector + ((cluster - 2) * headers.boot.bytesPerSec);
}

static uint32_t _get_cluster_entry(struct FAT32DirectoryEntry* entry){
	return (entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
}

// set 'out' with the 'name' formated in 8:3
static void _format_fat_name(const char* filename, char* out){
	memset(out, ' ', 11);

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

	for(int i = 0; i < 11; i++){
		if(out[i] >= 'a' && out[i] <= 'z'){
			out[i] -= 32;
		}
	}
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

		count++;

	} while(1);

	stream_dispose(stream);

	return count;
}

static struct Directory* _create_directory_entry(struct FAT32DirectoryEntry* entry){
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
	uint32_t lba = _cluster_to_lba(dir->firstCluster) * headers.boot.bytesPerSec;
	stream_seek(stream, lba);

	while(1){
		if(stream_read(stream, &buffer, sizeof(buffer)) != SUCCESS){
			stream_dispose(stream);
			return ERROR_IO;
		}

		uint8_t firstChar = buffer.DIR_Name[0];
		if(firstChar == 0x0){
			stream_dispose(stream);
			return FAILED;
		}

		if(firstChar == 0xE5){
			continue;
		}

		if(strncmp((char*)buffer.DIR_Name, filename, 11) == 0){
			stream_dispose(stream);
			
			struct FAT32DirectoryEntry* entry = _copy_fat_entry(&buffer);
			if(!entry){
				return NO_MEMORY;
			}

			enum ItemType type = (buffer.DIR_Attr & 0x10) ? Directory : File;
			itembuff->type = type;
			itembuff->file = entry;

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
	memset(fat, 0x0, sizeof(struct FAT));

	fat->readStream = stream_new();
	if(!fat->readStream){
		return NO_MEMORY;
	}

	fat->clusterReadStream = stream_new();
	if(!fat->clusterReadStream){
		return NO_MEMORY;
	}

	if(stream_read(fat->readStream, &fat->headers, sizeof(fat->headers)) != SUCCESS){
		return ERROR_IO;
	}

	headers = fat->headers;
	firstDataSector = headers.boot.rsvdSecCnt + (headers.boot.numFATs * headers.extended.FATSz32);

	int status = _get_root_directory(fat);
	if(status != SUCCESS){
		return status;
	}

	return SUCCESS;
}

struct FATFileDescriptor* FAT32_open(struct FAT* fat, const char *pathname, uint8_t flags, uint8_t mode){
	if(strlen(pathname) == 0 || strlen(pathname) > PATH_MAX){
		return 0x0;
	}

	struct FATFileDescriptor* fd = kmalloc(sizeof(struct FATFileDescriptor));
	if(!fd){
		return 0x0;
	}

	memset(fd, 0x0, sizeof(struct FATFileDescriptor));
		
	char path[PATH_MAX];
	memset(path, 0x0, sizeof(path)); // remove garbage
	strcpy(path, pathname);

	char* token = strtok(path, "/");

	struct FATItem item;

	do{
		int res = _get_item_in_diretory("kernel.bin", &item, &fat->rootDir);
		if(res != SUCCESS) break;

		terminal_write("\n--file--\n");
		terminal_write("type:    %s\n", item.type == File ? "File" : "Directory");
		
		if(item.type == File){
			terminal_write("name:    %s\n", item.file->DIR_Name);
			terminal_write("HighC:   %d\n", item.file->DIR_FstClusHI);
			terminal_write("LowC:    %d\n", item.file->DIR_FstClusLO);
			terminal_write("Size:    %d Bytes\n", item.file->DIR_FileSize);
		}

	} while ((token = strtok(0x0, "/")));

	return 0x0;
}

int FAT32_read(struct FATFileDescriptor *ffd, void *buffer, uint32_t count){
	return -1;
}

int FAT32_close(struct FATFileDescriptor *ffd){
	if(!ffd)
		return FAILED;

	return SUCCESS;
}
