#ifndef _STREAM_H
#define _STREAM_H

#include <device.h>
#include <stdint.h>

#define SECTOR_SIZE 512
#define SEEK_SET 0
#define SEEK_CUR 1

struct Stream{
	uint64_t cursor;
	uint8_t cache[SECTOR_SIZE];
	uint64_t cachedSectorLBA;
	uint8_t cacheValid;
	struct device* dev;
};

struct Stream* stream_new(struct device* device);
int stream_read(struct Stream* stream, void* buffer, int total);
int stream_write(struct Stream *stream, const void *buffer, int total);
int stream_seek(struct Stream* stream, uint64_t sector, uint8_t whence);
int stream_dispose(struct Stream* ptr);

#endif

