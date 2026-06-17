#include <def/compile.h>
#include <def/err.h>
#include <io/ports.h>
#include <asm/gdt.h>
#include <asm/idt.h>
#include <stdint.h>

#include "boot.h"
#include "uapi/headers.h"

#define ALIGN_UP(x,a) (((x) + ((a) - 1)) & ~((a) - 1))
#define BOOT_HEADER_ADDR (((int)_brk))

extern char _brk[];

/*
* Main module for the real-mode kernel code
*/

static void *boot_add_tag(uint8_t **offset, const struct boot_tag *tag){
	memcpy(*offset, tag, tag->size);
	void *ret = *offset;
	*offset += ALIGN_UP(tag->size, BOOT_TAG_ALIGN);
	return ret;
}

static void build_boot_header(
	struct boot_header* boot_header,
	const struct boot_tag** tags,
	unsigned int tags_count
){
	boot_header->signature = BOOT_HEADER_SIGNATURE;
	boot_header->version = 1;
	boot_header->size = sizeof(struct boot_header);
	boot_header->tags_size = 0;
	boot_header->tags_ptr = BOOT_HEADER_ADDR;
	boot_header->secure_boot = 0;

	const struct boot_tag* end;

	uint8_t *tags_ptr = (uint8_t *)boot_header->tags_ptr;
	for(int i = 0; i < tags_count; i++){
		if(tags[i]->type == BOOT_TAG_END){
			end = tags[i];
			continue;
		}

		boot_add_tag(&tags_ptr, tags[i]);
	}

	if(!end) return;

	struct boot_tag_e820 *memory = (struct boot_tag_e820*)tags_ptr;
	memory->tag.type = BOOT_TAG_E820;

	int written = detect_memory(memory->map);
	memory->entries = written;
	memory->tag.size = sizeof(struct boot_tag_e820) + (written * sizeof(struct e820_entry));	

	tags_ptr += ALIGN_UP(memory->tag.size, BOOT_TAG_ALIGN);

	boot_add_tag(&tags_ptr, end);

	boot_header->tags_size += tags_ptr - (uint8_t *)boot_header->tags_ptr;
	boot_header->tags_ptr += (ds() << 4);
}

// TODO: Implement missing
void main(){
	printf("Starting Real Mode kernel\n");

	struct boot_tag_video video;
	memset(&video, 0x0, sizeof(struct boot_tag_video));

	struct boot_tag_setup setup;
	memcpy(&setup, &hdr, sizeof(struct boot_tag_setup));
	
	// Check CPU

	setup_video(&video);

	// Build boot header
	struct boot_header boot_header;
	memset(&boot_header, 0x0, sizeof(struct boot_header));

	setup.tag.type = BOOT_TAG_SETUP;
	setup.tag.size = sizeof(struct boot_tag_setup);

	video.tag.type = BOOT_TAG_VIDEO;
	video.tag.size = sizeof(struct boot_tag_video);

	const struct boot_tag end = {.type = BOOT_TAG_END, .size = sizeof(struct boot_tag)};
	const struct boot_tag* tags[] = { &setup.tag, &video.tag, &end };

	build_boot_header(&boot_header, tags, ARRAY_SIZE(tags));

	if(IS_STAT_ERR(enable_a20())){
		printf("Failed to enable A20 line!\n");
		while(1) asm volatile ("hlt");
	}

	// mask all interrupts
	outb_p(0xFF, 0x21);
	outb_p(0xFF, 0xA1);

	// setup idt
	static const struct gdt_descriptor idt_null = {0, 0};
	asm volatile ("lidt %0" :: "m"(idt_null) : "memory");

	// setup gdt
	static const struct gdt_entry gdt[] = {
		GDT_ENTRY(0, 0, 0, 0),                                    // Null
		GDT_ENTRY(0, 0xFFFFF, GDT_CODE_RING0, GDT_FLAGS_DEFAULT), // Kernel code
		GDT_ENTRY(0, 0xFFFFF, GDT_DATA_RING0, GDT_FLAGS_DEFAULT), // Kernel data
	};

	static struct gdt_descriptor gdt_descriptor;
	gdt_descriptor.size = sizeof(gdt) - 1;
	gdt_descriptor.addr = (uintptr_t)gdt + (ds() << 4);

	gdt_load(&gdt_descriptor);

	go_to_protect_mode(
		setup.code32_start,
		(uint32_t)&boot_header + (ds() << 4)
	);

	__builtin_unreachable();
}

