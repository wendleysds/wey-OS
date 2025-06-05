#include <fs/fat/fat32.h>
#include <fs/fat/fatdefs.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <def/status.h>
#include <drivers/terminal.h>
#include <io/stream.h>
#include <stdint.h>

static int firstDataSector = -1; 
static struct FATHeaders headers;

static uint32_t _cluster_to_lba(uint32_t cluster){
	return firstDataSector + ((cluster - 2) * headers.boot.bytesPerSec);
}

static uint16_t _get_directory_itens_count(struct Directory* dir){
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

static int _get_root_directory(struct FAT* fat){
	fat->rootDir.firstCluster = headers.extended.rootClus;
	fat->rootDir.currentCluster = headers.extended.rootClus;

	uint16_t itemCount = _get_directory_itens_count(&fat->rootDir);
	if(itemCount < 0)
		return itemCount;

	uint32_t dirSize = sizeof(struct FAT32DirectoryEntry) * itemCount;
	struct FAT32DirectoryEntry* directoryEntry = (struct FAT32DirectoryEntry*)kcalloc(dirSize);

	if(!directoryEntry){
		return NO_MEMORY;
	}

	fat->rootDir.entry = directoryEntry;
	fat->rootDir.itensCount = itemCount;

	return SUCCESS;
}

int FAT32_init(struct FAT* fat){
	fat = (struct FAT*)kmalloc(sizeof(struct FAT));
	if(!fat){
		return NO_MEMORY;
	}

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
