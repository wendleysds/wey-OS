#include <platform.h>
#include <file.h>
#include <utils.h>
#include <def/err.h>
#include <headers.h>
#include <string.h>
#include <loader.h>

#define PAGE_SIZE 4096
#define SETUP_LOAD_ADDR 0x90000

int weyos_loader(entry_t* entry, fat_info_t* fat, struct load_info_struct* info_buffer){
	struct boot_tag_setup setup_header;

	struct file* file = platform_open_file(fat, entry->target);
	if(IS_ERR(file)){
		return PTR_ERR(file);
	}

	file->ops->lseek(file, 0, 0);
	file->ops->read(file, &setup_header, sizeof(struct boot_tag_setup));

	uint16_t setup_size =  setup_header.setup_sectors * 0x200;
	uint32_t start_kernel = (setup_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	uint32_t kernel_size = file->size - start_kernel;

	if((setup_header.syssize * 16) < kernel_size){
		printf("Kernel size is larger than syssize!\n");
		return FILE_TOO_LARGE;
	}

	if(setup_header.boot_sig != 0xAA55){
		printf("Invalid signature! %x\n", setup_header.boot_sig );
		return INVALID_FILE;
	}
	
	char buffer[4096];

	uint64_t bytes_readed = 0;
	uint64_t readed = 0;
	uint64_t load_address = SETUP_LOAD_ADDR;

	file->ops->lseek(file, 0x0, 0);

	memset(
		(void*)(SETUP_LOAD_ADDR + (setup_header.setup_sectors + 1) * 512), 
		0x0, 
		(64 - (setup_header.setup_sectors  + 1)) * 512
	);

	while((bytes_readed = file->ops->read(file, buffer, sizeof(buffer))) != END_OF_FILE){
		if(IS_STAT_ERR(bytes_readed)){
			return bytes_readed;
		}

		if(readed >= setup_size){
			break;
		}

		memcpy((void*)load_address, buffer, bytes_readed);
		readed += bytes_readed;
		load_address += bytes_readed;
	}

	file->ops->lseek(file, start_kernel, 0);

	bytes_readed = 0;
	load_address = setup_header.pref_address;
	while((bytes_readed = file->ops->read(file, buffer, sizeof(buffer))) != END_OF_FILE){
		if(IS_STAT_ERR(bytes_readed)){
			return bytes_readed;
		}

		if(bytes_readed >= kernel_size){
			break;
		}

		memcpy((void*)load_address, buffer, bytes_readed);
		load_address += bytes_readed;
	}
	
	file->ops->close(file);

	if(entry->initrd){
		// TODO: load initrd
	}

	info_buffer->entry = entry;
	info_buffer->entry_point = setup_header.code16_start;
	info_buffer->flags = SET_SEGMENTS;
	info_buffer->code_segment = SETUP_LOAD_ADDR >> 4;
	info_buffer->kerne_data = 0x0;

	return SUCCESS;
}
