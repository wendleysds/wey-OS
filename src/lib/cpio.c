#include <lib/string.h>
#include <lib/cpio.h>
#include <def/status.h>

struct cpio_header{
	char magic[6];  // "070701"
	char ino[8];
	char mode[8];
	char uid[8];
	char gid[8];
	char nlink[8];
	char mtime[8];
	char filesize[8];
	char devmajor[8];
	char devminor[8];
	char rdevmajor[8];
	char rdevminor[8];
	char namesize[8];
	char check[8];
} __packed;

static inline uintptr_t align4(uintptr_t p) {
	uintptr_t v = p;
	uintptr_t pad = (4 - (v % 4)) % 4;
	return v + pad;
}

static inline int cpio_range_valid(const uint8_t* start, const uint8_t* end, size_t len) {
	return len <= (size_t)(end - start);
}

static uint32_t cpio_parse_field(const char field[8]) {
	uint32_t value = 0;

	for(int i = 0; i < 8; i++) {
		value <<= 4;

		char c = field[i];

		if(c >= '0' && c <= '9')
			value |= c - '0';
		else if(c >= 'A' && c <= 'F')
			value |= c - 'A' + 10;
		else if(c >= 'a' && c <= 'f')
			value |= c - 'a' + 10;
	}

	return value;
}

int cpio_initramfs_iterate(void* initrd_start, size_t size, const uint8_t** cursor, struct cpio_file_iter* file_buffer){
	if (!initrd_start || !cursor || !file_buffer)
		return INVALID_ARG;

	uintptr_t aligned;
	const uint8_t* cur;

	uint8_t* start = (uint8_t*)initrd_start;
	if(size > UINTPTR_MAX - (uintptr_t)start){
		return OVERFLOW;
	}

	uint8_t* end = start + size;

	if (*cursor == NULL)
		cur = start;
	else
		cur = *cursor;

	if (!cpio_range_valid(cur, end, sizeof(struct cpio_header))){
		return INVALID_FILE;
	}

	const struct cpio_header* header = (const struct cpio_header*)cur;
	if (memcmp(header->magic, "070701", 6) != 0){
		return INVALID_FORMAT;
	}

	uint32_t namesize = cpio_parse_field(header->namesize);
	if (namesize == 0){
		return INVALID_FILE;
	}

	uint32_t filesize = cpio_parse_field(header->filesize);
	uint32_t ino = cpio_parse_field(header->ino);
	uint32_t mode = cpio_parse_field(header->mode);
	uint32_t uid = cpio_parse_field(header->uid);
	uint32_t gid = cpio_parse_field(header->gid);
	uint32_t nlink = cpio_parse_field(header->nlink);
	uint32_t mtime = cpio_parse_field(header->mtime);
	uint32_t devmajor = cpio_parse_field(header->devmajor);
	uint32_t devminor = cpio_parse_field(header->devminor);
	uint32_t rdevmajor = cpio_parse_field(header->rdevmajor);
	uint32_t rdevminor = cpio_parse_field(header->rdevminor);
	uint32_t check = cpio_parse_field(header->check);

	cur += sizeof(*header);

	if (!cpio_range_valid(cur, end, namesize)){
		return INVALID_FILE;
	}

	if (((char*)cur)[namesize - 1] != '\0'){
		return INVALID_STATE;
	}

	file_buffer->name = (const char*)cur;

	if (
		namesize == 11 &&
		memcmp(cur, "TRAILER!!!", 11) == 0
	) {
		return END_OF_FILE;
	}

	cur += namesize;

	aligned = align4((uintptr_t)cur);
	if (aligned > (uintptr_t)end)
		return OVERFLOW;

	cur = (uint8_t*)aligned;

	if (!cpio_range_valid(cur, end, filesize)){
		return INVALID_FILE;
	}

	file_buffer->content_ptr = cur;
	file_buffer->filesize = filesize;
	file_buffer->ino = ino;
	file_buffer->mode = mode;
	file_buffer->uid = uid;
	file_buffer->gid = gid;
	file_buffer->nlink = nlink;
	file_buffer->mtime = mtime;
	file_buffer->devmajor = devmajor;
	file_buffer->devminor = devminor;
	file_buffer->rdevmajor = rdevmajor;
	file_buffer->rdevminor = rdevminor;
	file_buffer->check = check;

	cur += filesize;

	aligned = align4((uintptr_t)cur);
	if (aligned > (uintptr_t)end){
		return INVALID_FILE;
	}

	cur = (uint8_t*)aligned;

	*cursor = cur;
	return OK;
}
