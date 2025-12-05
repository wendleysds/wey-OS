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
#define DEFAULT_TIMEOUT 30

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
	int16_t timeoutSecs;
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
	printf("Press any key to continue...\n");
	platform_timeout(DEFAULT_TIMEOUT);
	platform_die();
}

void main(){
	int status = platform_init();
	if (IS_STAT_ERR(status)){
		printf("Failer to init platform! %d\n", status);
		_die();
	}

	platform_init_video();

	platform_clear_screen();

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

	printf("| NO\n");

	printf("Searching config in %s ", path2);
	config_file = platform_open_file(fat, path2);
	if(!IS_ERR(config_file)) goto found;

	printf("| NO\n");

	printf("\nConfig file not found!\n");
	_die();

found:
	printf("| OK\n");

	/*
	Config file format ex:

	TIMEOUT 30 # time to select entry

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

	config.timeoutSecs = DEFAULT_TIMEOUT;

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

		// TIMEOUT
		if(strncmp(line, "TIMEOUT", 7) == 0){
			char* val = line + 7;
			while(IS_BLANK(*val)) val++;
			config.timeoutSecs = atoi(val);
			continue;
		}

		// DEFAULT
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

	// Make bootmenu

	uint8_t width, heigth;
	platform_get_resolution(&width, &heigth);
	platform_clear_screen();

	const uint8_t centerX = width / 2;
	const uint8_t centerY = heigth / 2;

	// Insane math(and magic numbers) to center everything
	// magic number == margin

	// end
	/*

	======== NoVaLoader =======

	         1. WeyOs *
		     2. Linux

	===========================

	*/

	const char* header = "======== NoVaLoader =======";
	const char* footer = "===========================";

	const uint8_t foo_hea_center_x = centerX - (strlen(footer) / 2) - 1;

	const uint8_t header_center_y = centerY - (config.count / 2) - 2;
	const uint8_t footer_center_y = header_center_y + config.count + 3;

	platform_set_cursor(foo_hea_center_x, header_center_y);
	printf(header);
	
	platform_set_cursor(foo_hea_center_x, footer_center_y);
	printf(footer);

	uint8_t yStart = centerY - (config.count / 2);

	int i = 0;
	int selected_index = 0;

	struct cords{
		uint8_t x, y;
	} bestSpotForSelect[config.count];

	cur = config.entries;
	do {
		// strlen("%d." + " " + LABEL);
		const uint8_t textSize = 2 + 1 + strlen(cur->label);
		const uint8_t middle = centerX - (textSize / 2);

		platform_set_cursor(middle, yStart);
		printf("%d. %s", i+1, cur->label);

		bestSpotForSelect[i].x = centerX + (textSize / 2) + 1;
		bestSpotForSelect[i].y = yStart;

		if(cur == config.def){
			platform_set_cursor(bestSpotForSelect[i].x, bestSpotForSelect[i].y);
			printf("*");
			selected_index = i;
		}

		yStart++;
		i++;
	}while((cur = cur->next));

	// help text
	const char* keys = "w: up, s: down, enter: select";
	const uint8_t keys_center_x = centerX - (strlen(keys) / 2) - 1;
	const uint8_t keys_center_y = footer_center_y + 1;
	platform_set_cursor(keys_center_x, keys_center_y);
	printf(keys);

	const int KEY_ENTER = 0xD;
	const int KEY_UP    = 0x77;
	const int KEY_DOWN  = 0x73;

	int16_t timeout = config.timeoutSecs;
	const uint8_t timeout_x = centerX;
	const uint8_t timeout_y = keys_center_y + 2;

	while(timeout >= 0){
		int k = platform_timeout(1);

		if(k == KEY_ENTER){
			break;
		}

		if(k == KEY_DOWN){
			// move down in the list
			if(selected_index + 1 < config.count){
				// erase previous marker
				platform_set_cursor(
					bestSpotForSelect[selected_index].x,
					bestSpotForSelect[selected_index].y
				);
				printf(" ");

				selected_index++;

				platform_set_cursor(
					bestSpotForSelect[selected_index].x,
					bestSpotForSelect[selected_index].y
				);
				printf("*");
			}
		}else if(k == KEY_UP){
			// move up in the list
			if(selected_index - 1 >= 0){
				platform_set_cursor(
					bestSpotForSelect[selected_index].x,
					bestSpotForSelect[selected_index].y
				);
				printf(" ");

				selected_index--;

				platform_set_cursor(
					bestSpotForSelect[selected_index].x,
					bestSpotForSelect[selected_index].y
				);
				printf("*");
			}
		}else if(k == 0){
			platform_set_cursor(timeout_x, timeout_y);
			if(timeout < 10) printf("0");
			printf("%d", timeout);
			timeout--;
		}
	}
	

	for(i = 0, cur = config.entries; i != selected_index && (cur = cur->next); i++);

	platform_clear_screen();
	printf("Selected: %s\n\n", cur->label);
	printf("Loading %s ...\n", cur->target);

	struct file* kernel_file = platform_open_file(fat, cur->target);
	if(IS_ERR(kernel_file)){
		printf("Failed to open %s!\n", cur->target);
		_die();
	}

	void* entry_point = NULL;

	status = weyos_loader(kernel_file, &entry_point, NULL);
	if(!IS_STAT_ERR(status)) goto ok;

	/*
	status = weyos_loader(kernel_file, &entry_point);
	if(!IS_STAT_ERR(status)) goto ok;
	*/

err_load:
	printf("Failed to load! %d\n", status);
	_die();

ok:
	if(cur->initrd){
		struct file* initrd_file = platform_open_file(fat, cur->initrd);
		if(IS_ERR(initrd_file)){
			status = PTR_ERR(initrd_file);
			goto err_load;
		}

		// TODO: implement load

		initrd_file->ops->close(initrd_file);
	}


	// TODO: jmp entry point

	__hlt;
}
