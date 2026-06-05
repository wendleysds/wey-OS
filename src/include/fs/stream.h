#ifndef _STREAM_H
#define _STREAM_H

#include <lib/list.h>
#include <device/blkdev.h>
#include <fs/vfs.h>
#include <stdint.h>

struct blkdev;

struct Stream{
	uint8_t* sector_cache;
	uint16_t sector_size;

	off_t pos;
	sector_t cachedSectorLBA;

	uint8_t cacheValid;
	uint8_t dirty;

	struct list_head tasks;
	struct blkdev* bdev;
};

struct Stream* stream_new(struct blkdev* bdev);
int stream_read(struct Stream* stream, void* buffer, int total);
int stream_write(struct Stream* stream, const void* buffer, int total);
off_t stream_seek(struct Stream* stream, off_t offset, uint8_t whence);

static inline off_t stream_tell(struct Stream* stream){
	return stream->pos;
}

static inline struct blkdev* stream_bdev(struct Stream* stream){
	return stream->bdev;
}

static inline off_t stream_seek_lba(struct Stream* stream, sector_t sector, uint8_t whence){
	return stream_seek(stream, sector * stream->sector_size, whence);
}

int stream_dispose(struct Stream* ptr);

#endif

