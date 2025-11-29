#include <utils.h>
#include <platform.h>
#include <def/err.h>
#include <disk.h>
#include <heap.h>

#define HEAP_LOW_ADDR 0x8000
#define HEAP_HIGH_ADDR 0x1000000

#define HEAP_SIZE  0x4000

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

void main(){
	int status = platform_init();
	if (IS_STAT_ERR(status)){
		printf("Failer to init platform! %d\n", status);
		platform_die();
	}

	printf("=== NoVaLoader v0.1 ===\n");

	struct e820_entry entries[32];
	uint32_t counter = 0;
	status = platform_get_memory_map(entries, ARRAY_SIZE(entries), &counter);

	if (IS_STAT_ERR(status)){
		printf("Failed to get memory map! %d\n", status);
		platform_die();
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
		platform_die();
	}

	printf("\n");

	status = platform_disk_init();
	
	if(IS_STAT_ERR(status)){
		printf("Failed to init disk! %d\n", status);
		platform_die();
	}

	status = disk_parse_partitions(main_disk);
	if(IS_STAT_ERR(status)){
		printf("Failed to parse disk partitions! %d\n", status);
		platform_die();
	}

	printf("\nParsing FAT...\n");

	fat_info_t* fat = platform_parse_fat(main_disk);
	if(IS_ERR(fat)){
		printf("Failed to parse FAT! %d\n", PTR_ERR(fat));
		platform_die();
	}

	printf("Vol Name: %s\nBootSig: %x\n", fat->headers.boot.OEMName, fat->headers.fat32.bootSig);
	printf("Vol Labe: %s\nvolID: %x\n", fat->headers.fat32.volLab, fat->headers.fat32.volID);

	const char* path1 = "/boot/boot.cfg";
	const char* path2 = "/boot.cfg";

	struct file* config_file = NULL;

	printf("\nSearching config in %s\n", path1);
	config_file = platform_open_file(fat, path1);
	if(!IS_ERR(config_file)) goto found;

	printf("Searching config in %s\n", path2);
	config_file = platform_open_file(fat, path2);
	if(!IS_ERR(config_file)) goto found;

	printf("Config file not found!");
	platform_die();

found:
	printf("Found, parsing...\n");

	char buffer[512];
	config_file->ops->read(config_file, buffer, 32);
	printf(buffer);

	__hlt;
}