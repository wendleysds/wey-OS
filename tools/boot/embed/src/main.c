#include <utils.h>
#include <platform.h>
#include <def/err.h>
#include <disk.h>
#include <heap.h>
#include <loader.h>

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

	/*
	Config file format:
	KEY=value
	ex:

	/*
	Config file format:
	KEY=value
	ex:

	TARGET=/boot/kernel
	INITRD=/boot/ramfs.img
	*/

	char *config_buf = NULL;
	uint32_t cfg_size = config_file->size;
	config_buf = malloc(cfg_size + 1);
	if(!config_buf){
		printf("Failed to allocate memory for config\n");
		platform_die();
	}

	uint32_t read_total = 0;
	while(read_total < cfg_size){
		int r = config_file->ops->read(config_file, config_buf + read_total, cfg_size - read_total);
		if(r < 0){
			printf("Error reading config file: %d\n", r);
			free(config_buf);
			platform_die();
		}
		if(r == 0) break;
		read_total += r;
	}
	config_buf[read_total] = '\0';

	char *target_path = NULL;
	char *initrd_path = NULL;

	char *line = config_buf;
	char *end = config_buf + read_total;

	while(line < end){
		char *nl = strchr(line, '\n');
		char *line_end = nl ? nl : end;
		/* trim trailing CR/LF and spaces */
		while(line_end > line && (*(line_end-1) == '\r' || *(line_end-1) == ' ' || *(line_end-1) == '\t')){
			*(--line_end) = '\0';
		}
		/* skip empty lines or comments */
		if(line_end == line || *line == '#'){
			line = nl ? (nl + 1) : end;
			continue;
		}

		/* find '=' */
		char *eq = NULL;
		for(char *p = line; p < line_end; ++p){
			if(*p == '='){ eq = p; break; }
		}
		if(!eq){
			line = nl ? (nl + 1) : end;
			continue;
		}

		*eq = '\0';
		char *key = line;
		char *val = eq + 1;

		/* trim leading spaces on key */
		while(*key == ' ' || *key == '\t') key++;
		/* trim trailing spaces on key */
		char *kend = eq - 1;
		while(kend > key && (*kend == ' ' || *kend == '\t')){ *kend = '\0'; kend--; }

		/* trim leading spaces on val */
		while(*val == ' ' || *val == '\t') val++;
		/* trim trailing spaces on val */
		char *vend = line_end - 1;
		while(vend > val && (*vend == ' ' || *vend == '\t')){ *vend = '\0'; vend--; }

		if(strcmp(key, "TARGET") == 0){
			if(target_path) free(target_path);
			target_path = strdup(val);
		} else if(strcmp(key, "INITRD") == 0){
			if(initrd_path) free(initrd_path);
			initrd_path = strdup(val);
		}

		line = nl ? (nl + 1) : end;
	}

	/* close config file */
	if(config_file->ops && config_file->ops->close) config_file->ops->close(config_file);

	if(!target_path){
		printf("No TARGET specified in config\n");
		free(config_buf);
		platform_die();
	}

	printf("Target: %s\n", target_path);
	if(initrd_path) printf("Initrd: %s\n", initrd_path);

	/* open kernel file and pass to loader */
	struct file *kernel_file = platform_open_file(fat, target_path);
	if(IS_ERR(kernel_file)){
		printf("Failed to open kernel file %s\n", target_path);
		free(config_buf);
		platform_die();
	}

	weyos_loader(kernel_file);

	/* cleanup */
	if(kernel_file->ops && kernel_file->ops->close) kernel_file->ops->close(kernel_file);
	free(config_buf);
	if(target_path) free(target_path);
	if(initrd_path) free(initrd_path);

	//-----------------------------------

	/* Read entire config file into a buffer */
	uint32_t fsize = config_file->size;
	char *buf = malloc(fsize + 1);
	if (!buf) {
		printf("Out of memory while reading config\n");
		platform_die();
	}

	ssize_t got = file_read(config_file, buf, fsize);
	if (got < 0) {
		printf("Failed to read config file: %d\n", (int)got);
		platform_die();
	}
	
	buf[got] = '\0';

	/* Parse lines KEY=value */
	char *target = NULL;
	char *initrd = NULL;

	char *saveptr = NULL;
	char *line = strtok_r(buf, "\r\n", &saveptr);
	while (line) {
		/* trim leading whitespace */
		while (*line == ' ' || *line == '\t') line++;
		if (*line == '\0' || *line == '#') {
			line = strtok_r(NULL, "\r\n", &saveptr);
			continue;
		}

		char *eq = strchr(line, '=');
		if (!eq) {
			line = strtok_r(NULL, "\r\n", &saveptr);
			continue;
		}
		*eq = '\0';
		char *key = line;
		char *val = eq + 1;

		/* trim trailing whitespace from key */
		char *kend = key + strlen(key) - 1;
		while (kend >= key && (*kend == ' ' || *kend == '\t')) {
			*kend = '\0';
			kend--;
		}

		/* trim leading and trailing whitespace from val */
		while (*val == ' ' || *val == '\t') val++;
		char *vend = val + strlen(val) - 1;
		while (vend >= val && (*vend == ' ' || *vend == '\t')) {
			*vend = '\0';
			vend--;
		}

		if (strcmp(key, "TARGET") == 0) {
			target = malloc(strlen(val) + 1);
			if (target) strcpy(target, val);
		} else if (strcmp(key, "INITRD") == 0) {
			initrd = malloc(strlen(val) + 1);
			if (initrd) strcpy(initrd, val);
		}

		line = strtok_r(NULL, "\r\n", &saveptr);
	}

	free(buf);

	if (!target) {
		printf("Config missing TARGET entry\n");
		platform_die();
	}

	printf("Config -> TARGET=%s INITRD=%s\n", target, initrd ? initrd : "(none)");

	/* Open kernel file */
	struct file* kernel_file = platform_open_file(fat, target);
	if (IS_ERR(kernel_file)) {
		printf("Failed to open kernel '%s' (%d)\n", target, PTR_ERR(kernel_file));
		platform_die();
	}

	/* Open initrd if specified */
	struct file* initrd_file = NULL;
	if (initrd) {
		initrd_file = platform_open_file(fat, initrd);
		if (IS_ERR(initrd_file)) {
			printf("Failed to open initrd '%s' (%d)\n", initrd, PTR_ERR(initrd_file));
			platform_die();
		}
	}

	/* Free temporary strings */
	free(target);
	if (initrd) free(initrd);

	/* Hand off to loader */
	weyos_loader(kernel_file);

	/* If loader returns, halt */
	platform_die();

	__hlt;
}
