#include "drivers/terminal.h"
#include "fs/fat/fatdefs.h"
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fat/fat32.h>
#include <lib/mem.h>
#include <core/kernel.h>
#include <def/status.h>
#include <def/config.h>
#include <stdint.h>

static struct FileDescriptor* fileDescriptors[FILE_DESCRIPTORS_MAX];

typedef struct FATFileDescriptor FILE;

void fs_init() {
	memset(fileDescriptors, 0x0, sizeof(fileDescriptors));

	struct FAT fat;
	int status = FAT32_init(&fat); 

	if(status != SUCCESS){
		panic("Failed to initialize File System! STATUS %d", status);
	}

	char* filepath = "/home/test.txt";

	FILE* file = FAT32_open(&fat, filepath, 0, 0);	
	if(!file)
		terminal_write("%s not found!\n", filepath);

	char buffer[512];

	terminal_write("\nReading content from '%s'...\n");
	int bytesReaded = FAT32_read(&fat, file, buffer, sizeof(buffer));
	terminal_write("Readed %d bytes\n", bytesReaded);
	terminal_write("%s: ", filepath);

	for(int i = 0; i < bytesReaded; i++)
		terminal_write("%c", buffer[i]);
}


