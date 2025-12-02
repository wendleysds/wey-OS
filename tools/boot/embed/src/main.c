#include <utils.h>
#include <platform.h>
#include <def/err.h>
#include <disk.h>
#include <heap.h>
#include <loader.h>
#include <string.h>

#define MAX_CONFIG_SIZE 4096

#define HEAP_LOW_ADDR 0x8000
#define HEAP_HIGH_ADDR 0x1000000

#define HEAP_SIZE  0x4000

#define IS_BLANK(c) ((c)==' ' || (c)=='\t' || (c)=='\n' || (c)=='\r')

typedef struct entry{
	char* label;
	char* target;
	char* initrd;
	struct entry* next;
} entry_t;

typedef struct {
	entry_t* entries;
	uint16_t count;
	entry_t* def;
} config_t;

static uint8_t usable_ram_area(struct e820_entry* entries, uint32_t count, uint32_t start, uint32_t end){
	for(uint32_t i = 0; i < count; i++){
		if(entries[i].type == E820_TYPE_RAM){
			uint32_t region_start = (uint32_t)(entries[i].base_addr & 0xFFFFFFFF);
			uint32_t region_end = region_start + (uint32_t)(entries[i].length & 0xFFFFFFFF);

			if(region_start <= start && region_end >= end){
				return 1;
			}
		}
	}

	return 0;
}

void _die(){
	const int timeout = 10;
	printf("Press any key to continue... or wait %d seconds", timeout);
	platform_timeout(timeout);
	platform_die();
}

void main(){
	int status = platform_init();
	if (IS_STAT_ERR(status)){
		printf("Failer to init platform! %d\n", status);
		_die();
	}

	printf("=== NoVaLoader v0.1 ===\n");

	struct e820_entry entries[32];
	uint32_t counter = 0;
	status = platform_get_memory_map(entries, ARRAY_SIZE(entries), &counter);

	if (IS_STAT_ERR(status)){
		printf("Failed to get memory map! %d\n", status);
		_die();
	}

	printf("Memory Map Entries: %d\n", counter);
	if(usable_ram_area(entries, counter, HEAP_LOW_ADDR, HEAP_LOW_ADDR + HEAP_SIZE)){
		heap_init((void*)HEAP_LOW_ADDR, HEAP_SIZE);
		printf("Heap initialized at 0x%x - 0x%x\n", HEAP_LOW_ADDR, HEAP_LOW_ADDR + HEAP_SIZE);
	} else if(usable_ram_area(entries, counter, HEAP_HIGH_ADDR, HEAP_HIGH_ADDR + HEAP_SIZE)){
		heap_init((void*)HEAP_HIGH_ADDR, HEAP_SIZE);
		printf("Heap initialized at 0x%x - 0x%x\n", HEAP_HIGH_ADDR, HEAP_HIGH_ADDR + HEAP_SIZE);
	}else{
		printf("No usable RAM area found for heap!\n");
		_die();
	}

	printf("\n");

	status = platform_disk_init();
	
	if(IS_STAT_ERR(status)){
		printf("Failed to init disk! %d\n", status);
		_die();
	}

	status = disk_parse_partitions(main_disk);
	if(IS_STAT_ERR(status)){
		printf("Failed to parse disk partitions! %d\n", status);
		_die();
	}

	printf("\nParsing FAT...\n");

	fat_info_t* fat = platform_parse_fat(main_disk);
	if(IS_ERR(fat)){
		printf("Failed to parse FAT! %d\n", PTR_ERR(fat));
		_die();
	}

	printf("Vol Name: %s\nBootSig: %x\n", fat->headers.boot.OEMName, fat->headers.fat32.bootSig);
	printf("Vol Labe: %s\nvolID: %x\n", fat->headers.fat32.volLab, fat->headers.fat32.volID);

	const char* path1 = "/boot/boot.cfg";
	const char* path2 = "/boot.cfg";

	struct file* config_file = NULL;

	printf("\nSearching config in %s ", path1);
	config_file = platform_open_file(fat, path1);
	if(!IS_ERR(config_file)) goto found;

	printf("\nSearching config in %s ", path2);
	config_file = platform_open_file(fat, path2);
	if(!IS_ERR(config_file)) goto found;

	printf("\nConfig file not found!\n");
	_die();

found:
	printf("| OK\n");

	/*
	Config file format ex:

	DEFAULT WeyOs # default boot entry after timeout

	# My OS
	LABEL WeyOs
		TARGET=/boot/kernel
		INITRD=/boot/ramfs.img

	# other entry
	LABEL linux
		TARGET=/boot/vmlinux # kernel
		INITRD=/boot/initramfs.cpuio

	*/

	char* buffer = NULL;
	uint16_t buffer_size = 0;

	if(config_file->size > MAX_CONFIG_SIZE){
		printf("Config file too large! parsing only %d bytes", MAX_CONFIG_SIZE);
		buffer_size = MAX_CONFIG_SIZE;
	}else{
		buffer_size = config_file->size;
	}

	buffer = malloc(buffer_size);
	if(!buffer){
		printf("Failed to allocate memory for config\n");
		_die();
	}

	// Read the whole file to memory
	uint32_t readed = config_file->ops->read(config_file, buffer, buffer_size);
	if(IS_STAT_ERR(readed)){
		printf("Error reading cfg file! %d", readed);
		_die();
	}

	config_file->ops->close(config_file);
	config_file = NULL;

	config_t config;
	memset(&config, 0x0, sizeof(config));

	char* p = buffer;
	char* end = buffer + buffer_size;
	entry_t* current = NULL;

	while(p < end){
		// Jump space and empty lines
		while(p < end && IS_BLANK(*p)) p++;
		if(p >= end) break;

		char* line = p;

		while(p < end && *p != '\n' && *p != '\r') p++;

		char* line_end = p;
		*line_end = '\0'; // End the line
		p++;

		// Handle comments
		for(char* p = line; *p; p++){
			if(*p == '#'){
				*p = '\0';
				return;
			}
		}

		// If line was empty after removing comment, ignore :D
		for(char* t=line; *t; t++){
			if(!IS_BLANK(*t)) goto process_line;
		}

		continue;

process_line:

		if(strncmp(line, "DEFAULT", 7) == 0){
			char* val = line + 7;
			while(IS_BLANK(*val)) val++;
			config.def = (entry_t*)val; // tmp
			continue;
		}

		// LABEL
		if(strncmp(line, "LABEL", 5) == 0){
			char* val = line + 5;
			while(IS_BLANK(*val)) val++;

			current = malloc(sizeof(entry_t));
			if(config.entries == NULL){
				config.entries = current;
			}else{
				current->next = config.entries;
				config.entries = current;
			}

			current->label = val;
			config.count++;

			continue;
		}

		// TARGET=
		if(current && strncmp(line, "TARGET=", 7) == 0){
			current->target = line + 7;
			while(IS_BLANK(*current->target)) current->target++;
			continue;
		}

		// INITRD=
		if(current && strncmp(line, "INITRD=", 7) == 0){
			current->initrd = line + 7;
			while(IS_BLANK(*current->initrd)) current->initrd++;
			continue;
		}
	}

	// Resolve default
	if(config.def && (uintptr_t)config.def > 0xFFF){
		entry_t* cur = config.entries;

		do{
			// config.def points to label text
			if(strcmp(cur->label, (char*)config.def) == 0){
				config.def = cur;
				break;
			}
		}while((cur = cur->next));

	}else if(config.count > 0){
		config.def = config.entries; // fallback
	}

	printf("Parsed %d entries.\n\n", config.count);

	entry_t* cur = config.entries;

	do{
		printf("label: %s\n", cur->label);
		printf("    target: %s\n", cur->target);
		printf("    initrd: %s\n\n", cur->initrd ? cur->initrd : "NULL");
	}while((cur = cur->next));

	__hlt;
}
