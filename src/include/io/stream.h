#ifndef _STREAM_H
#define _STREAM_H

#include <stdint.h>

struct Stream{
	uint32_t unused;
};

struct Stream* stream_new();
int stream_read(struct Stream* stream, void* buffer, int total);
void stream_seek(struct Stream* stream, uint32_t sector);
void stream_dispose(struct Stream* ptr);

#endif

