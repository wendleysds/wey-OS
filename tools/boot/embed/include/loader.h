#ifndef _LOADER_H
#define _LOADER_H

#define SET_SEGMENTS 0x1 // need set cs before jump (far jump)
#define CODE_32 0x2 // need go to pmmode 

typedef struct entry{
	char* label;
	char* target;
	char* initrd;
	struct entry* next;
} entry_t;

struct load_info_struct {
	uint64_t entry_point;
	uint8_t flags;
	uint16_t code_segment;
	uint32_t kerne_data;
	entry_t* entry;
};

int weyos_loader(entry_t* entry, fat_info_t* fat, struct load_info_struct* info_buffer);

#endif