#ifndef _STREAM_H
#define _STREAM_H

#include <wey/blkdev.h>
#include <wey/vfs.h>
#include <stdint.h>

#define SECTOR_SIZE 512
#define SEEK_SET 0
#define SEEK_CUR 1

struct Stream{
	uint8_t cache[SECTOR_SIZE];
	uint64_t cachedSectorLBA;
	uint8_t cacheValid;

	struct file fbdev;
	struct blkdev* bdev;
};

struct Stream* stream_new(struct blkdev* bdev);
int stream_read(struct Stream* stream, void* buffer, int total);
int stream_write(struct Stream *stream, const void *buffer, int total);
int stream_seek(struct Stream* stream, uint32_t offset, uint8_t whence);

static inline uint32_t stream_tell(struct Stream* stream){
	return stream->fbdev.pos;
}

int stream_dispose(struct Stream* ptr);

#endif

