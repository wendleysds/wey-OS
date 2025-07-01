#ifndef _STREAM_H
#define _STREAM_H

#include <stdint.h>

#define SECTOR_SIZE 512

struct Stream{
	uint32_t cursor;
	uint8_t cache[SECTOR_SIZE];
	uint32_t cachedSectorLBA;
	uint8_t cacheValid;
};

struct Stream* stream_new();
int stream_read(struct Stream* stream, void* buffer, int total);
int stream_write(struct Stream *stream, const void *buffer, int total);
int stream_seek(struct Stream* stream, uint32_t sector);
int stream_dispose(struct Stream* ptr);

#endif

