#ifndef _STREAM_H
#define _STREAM_H

#include <stdint.h>

#define SECTOR_SIZE 512

struct Stream{
	uint32_t unused;
	uint32_t size;
    uint8_t cache[SECTOR_SIZE];
    uint32_t cachedSectorLBA;
    uint8_t cacheValid;
	uint64_t cacheDirt;
};

struct Stream* stream_new();
int stream_read(struct Stream* stream, void* buffer, int total);
int stream_write(struct Stream *stream, void *buffer, int total);
void stream_seek(struct Stream* stream, uint32_t sector);
void stream_dispose(struct Stream* ptr);

#endif

