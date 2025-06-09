#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fat/fat32.h>
#include <lib/mem.h>
#include <core/kernel.h>
#include <def/status.h>
#include <def/config.h>
#include <stdint.h>

static struct FileDescriptor* fileDescriptors[FILE_DESCRIPTORS_MAX];

void fs_init() {
	memset(fileDescriptors, 0x0, sizeof(fileDescriptors));

	struct FAT fat;
	int status = FAT32_init(&fat); 

	if(status != SUCCESS){
		panic("Failed to initialize File System! STATUS %d", status);
	}

	FAT32_open(&fat, "/kernel.bin", 0, 0);	
}


