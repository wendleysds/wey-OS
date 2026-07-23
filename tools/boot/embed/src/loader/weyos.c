#include <platform.h>
#include <file.h>
#include <utils.h>
#include <def/errno.h>
#include <headers.h>
#include <string.h>
#include <loader.h>

#define PAGE_SIZE 4096
#define SETUP_LOAD_ADDR  0x90000
#define INITRD_LOAD_ADDR 0x20000

static int load_file_segment(struct file* file, uint32_t offset, void* dest, uint32_t size){
	char buffer[PAGE_SIZE];
	uint64_t bytes_read;
	uint32_t loaded = 0;
	char* target = dest;

	file->ops->lseek(file, offset, 0);

	while(loaded < size && (bytes_read = file->ops->read(file, buffer, sizeof(buffer))) > 0){
		if(IS_ERR_VALUE(bytes_read)){
			return (int)bytes_read;
		}

		uint32_t chunk = bytes_read;
		if(chunk > size - loaded){
			chunk = size - loaded;
		}

		memcpy(target + loaded, buffer, chunk);
		loaded += chunk;
	}

	return SUCCESS;
}

int weyos_loader(entry_t* entry, fat_info_t* fat, struct load_info_struct* info_buffer){
	struct boot_tag_setup setup_header;

	struct file* file = platform_open_file(fat, entry->target);
	if(IS_ERR(file)){
		return PTR_ERR(file);
	}

	printf("Loading %s ...\n", entry->target);
	file->ops->lseek(file, 0, 0);
	file->ops->read(file, &setup_header, sizeof(struct boot_tag_setup));

	uint16_t setup_size = setup_header.setup_sectors * 0x200;
	uint32_t start_kernel = (setup_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	uint32_t kernel_size = file->size - start_kernel;

	if((setup_header.syssize * 16) < kernel_size){
		printf("Kernel size is larger than syssize!\n");
		file->ops->close(file);
		return -EFBIG;
	}

	if(setup_header.boot_sig != 0xAA55){
		printf("Invalid signature! %x\n", setup_header.boot_sig);
		file->ops->close(file);
		return -EINVAL;
	}

	memset(
		(void*)(SETUP_LOAD_ADDR + (setup_header.setup_sectors + 1) * 512),
		0x0,
		(64 - (setup_header.setup_sectors + 1)) * 512
	);

	int ret = load_file_segment(file, 0, (void*)SETUP_LOAD_ADDR, setup_size);
	if(ret < 0){
		file->ops->close(file);
		return ret;
	}

	ret = load_file_segment(file, start_kernel, (void*)setup_header.pref_address, kernel_size);
	file->ops->close(file);
	if(ret < 0){
		return ret;
	}

	if(entry->initrd){
		printf("Loading %s ...\n", entry->initrd);

		file = platform_open_file(fat, entry->initrd);
		if(IS_ERR(file)){
			return PTR_ERR(file);
		}

		uint32_t initrd_size = (uint32_t)file->size;
		ret = load_file_segment(file, 0, (void*)INITRD_LOAD_ADDR, initrd_size);
		file->ops->close(file);
		if(ret < 0){
			return ret;
		}

		struct boot_tag_setup* setup = (void*)SETUP_LOAD_ADDR;
		setup->ramdisk_ptr = INITRD_LOAD_ADDR;
		setup->ramdisk_size = initrd_size;
	}

	info_buffer->entry = entry;
	info_buffer->entry_point = setup_header.code16_start;
	info_buffer->flags = SET_SEGMENTS;
	info_buffer->code_segment = SETUP_LOAD_ADDR >> 4;
	info_buffer->kerne_data = 0x0;

	return SUCCESS;
}
