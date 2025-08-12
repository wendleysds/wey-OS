#include <io/stream.h>
#include <drivers/ata.h>
#include <memory/kheap.h>
#include <lib/mem.h>

#include <def/status.h>

struct Stream* stream_new(struct device* device){
	if(!device) return 0x0;
	struct Stream* s = (struct Stream*)kmalloc(sizeof(struct Stream));
	if(!s) return 0x0;
	
	s->cursor = 0;
	s->cacheValid = 0;
	s->dev = device;
	memset(s->cache, 0x0, sizeof(s->cache));

	return s;
}

struct Stream* stream_dup(struct Stream* stream){
	struct Stream* s = (struct Stream*)kmalloc(sizeof(struct Stream));
	if(!s) return 0x0;
	return memcpy(s, stream, sizeof(struct Stream));
}

int stream_read(struct Stream *stream, void *buffer, int total){
	if(!stream || !buffer || total <= 0){
		return INVALID_ARG;
	}

	int totalRemaining = total;
	uint8_t* bufPtr = (uint8_t*)buffer;

	while(totalRemaining > 0){
		uint32_t sector = stream->cursor / SECTOR_SIZE;
		uint64_t offset = stream->cursor % SECTOR_SIZE;

		// Check if we need to read a new sector into the cache
		// If the cache is invalid or the sector is different, read it
		// from the disk
		if (!stream->cacheValid || stream->cachedSectorLBA != sector) {
			int status = stream->dev->read(
				stream->dev, stream->cache,
				sizeof(stream->cache), (stream->cursor &= ~(SECTOR_SIZE - 1))
			);

            if (status < 0) return status;

            stream->cachedSectorLBA = sector;
            stream->cacheValid = 1;
        }

		uint32_t available = SECTOR_SIZE - offset;
        uint32_t toRead = (totalRemaining < available) ? totalRemaining : available;

		memcpy(bufPtr, stream->cache + offset, toRead);

		// Update pointers and counters
		bufPtr += toRead;
		stream->cursor += toRead;
		totalRemaining -= toRead;
	}

	return SUCCESS;
}

int stream_write(struct Stream *stream, const void *buffer, int total){
	if(!stream || !buffer || total <= 0){
		return INVALID_ARG;
	}

	int totalRemaining = total;
	uint8_t* bufPtr = (uint8_t*)buffer;

	while(totalRemaining > 0){
		uint32_t sector = (stream->cursor & ~(SECTOR_SIZE - 1));
		uint64_t offset = stream->cursor % SECTOR_SIZE;

		char cache[SECTOR_SIZE];
		int readStatus = stream->dev->read(
			stream->dev, cache, 
			sizeof(cache), sector
		);

		if (readStatus < 0) return readStatus;

		uint32_t available = SECTOR_SIZE - offset;
		uint32_t toWrite = (totalRemaining < available) ? totalRemaining : available;

		memcpy(cache + offset, bufPtr, toWrite);

		int writeStatus = stream->dev->write(
			stream->dev, cache, 
			sizeof(cache), sector
		);

		if(writeStatus < 0) return writeStatus;

		// Update pointers and counters
		bufPtr += toWrite;
		stream->cursor += toWrite;
		totalRemaining -= toWrite;
	}

	return SUCCESS;
}

int stream_seek(struct Stream *stream, uint64_t sector, uint8_t whence){
	if(!stream || sector < 0){
		return INVALID_ARG;
	}

	switch (whence)
	{
		case SEEK_SET:
			stream->cursor = sector;
			stream->cacheValid = 0;
			break;
		case SEEK_CUR:
			stream->cursor += sector;
			break;
		default:
			return INVALID_ARG; // Invalid whence
			break;
	}

	return SUCCESS;
}

int stream_dispose(struct Stream *ptr){
	if(!ptr) return INVALID_ARG;

	kfree(ptr);

	return SUCCESS;
}
