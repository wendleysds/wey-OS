#include <fs/fat/fat32.h>
#include <fs/fat/fatdefs.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <def/status.h>
#include <drivers/terminal.h>
#include <io/stream.h>
#include <stdint.h>

static uint16_t _get_dir_itens_count(uint32_t dirStartSector){
	return 0;
}

static int _list_root_dir(struct FAT* fat){
	struct FATHeaders* h = &fat->headers;
	
	uint32_t firstDataSector = h->boot.rsvdSecCnt + (h->boot.numFATs * h->extended.FATSz32);
	uint32_t rootSector = firstDataSector + ((h->extended.rootClus - 2) * h->boot.secPerClus);

	terminal_write("\n");
	terminal_write("First Data Sector: %d\n",firstDataSector);
	terminal_write("Root Sector:       %d\n", rootSector);

	uint8_t buffer[512];
	
	// bytes offset = sector * bytesPerSector
	stream_seek(fat->readStream, rootSector * fat->headers.boot.bytesPerSec);
	stream_read(fat->readStream, buffer, sizeof(buffer));
	terminal_write("\n");

	struct FATDirectoryEntry* entries = (struct FATDirectoryEntry*)buffer;

	for (int i = 0; i < 16; i++){
		if (entries[i].DIR_Name[0] == 0x00) break;
        if (entries[i].DIR_Name[0] == 0xE5) continue;
		if (entries[i].DIR_FileSize == -1) continue;

		char type = (entries[i].DIR_Attr & 0x10) ? 'D' : 'F';

		char name[12] = {0};
        memcpy(name, entries[i].DIR_Name, 11);
        name[11] = '\0';

		terminal_write("[%c] %s (%d bytes)\n", type, name, entries[i].DIR_FileSize);
	}

	return FAILED;
}

int fs_init(){
	struct FAT* fat = (struct FAT*)kmalloc(sizeof(struct FAT));
	if(!fat){
		return NO_MEMORY;
	}

	memset(fat, 0x0, sizeof(struct FAT));

	fat->readStream = stream_new();
	if(!fat->readStream){
		kfree(fat);
		return NO_MEMORY;
	}

	fat->clusterReadStream = stream_new();
	if(!fat->clusterReadStream){
		stream_dispose(fat->readStream);
		kfree(fat);
		return NO_MEMORY;
	}

	if(stream_read(fat->readStream, &fat->headers, sizeof(fat->headers)) != SUCCESS){
		stream_dispose(fat->readStream);
		stream_dispose(fat->clusterReadStream);
		kfree(fat);
		return ERROR_IO;
	}

	_list_root_dir(fat);

	return SUCCESS;
}
