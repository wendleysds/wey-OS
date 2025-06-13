#include "drivers/terminal.h"
#include "fs/fat/fatdefs.h"
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fat/fat32.h>
#include <lib/mem.h>
#include <lib/string.h>
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

	char* filepath = "/home/chars.txt";

	FILE* file = FAT32_open(&fat, filepath, 0, 0);

	if(!file){
		terminal_write("%s not found!\n", filepath);
		return;
	}

	terminal_write("Reading %s...\n", file->item->file->DIR_Name);

	char buffer[5000];
	int bytesReaded = 0;
	int totalReaded = 0;
	int lastReadAmount = 0;
	while((bytesReaded = FAT32_read(&fat, file, buffer, 50)) > 0){
		totalReaded += bytesReaded;
		lastReadAmount = bytesReaded;
	}

	terminal_write("Total read: %d bytes\n", totalReaded);

	terminal_write("Reading Last %d bytes\n", lastReadAmount);
	for (int i = 0; i < lastReadAmount; i++) {
		terminal_write("%c", buffer[i]);
	}

	terminal_write("\n");

	FAT32_close(file);
}

