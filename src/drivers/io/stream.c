#include <io/stream.h>
#include <io/ata.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <drivers/terminal.h>

#include <def/status.h>

struct Stream* stream_new(){
	struct Stream* s = (struct Stream*)kmalloc(sizeof(struct Stream));
	if(!s) return 0x0;
	
	s->cursor = 0;
	s->cacheValid = 0;

	return s;
}

int stream_read(struct Stream *stream, void *buffer, int total){
	if(!stream || !buffer || total <= 0){
		return INVALID_ARG;
	}

	int totalRemaining = total;
	uint8_t* bufPtr = (uint8_t*)buffer;

	while(totalRemaining > 0){
		int sector = stream->cursor / SECTOR_SIZE;
		int offset = stream->cursor % SECTOR_SIZE;

		// Check if we need to read a new sector into the cache
		// If the cache is invalid or the sector is different, read it
		// from the disk
		if (!stream->cacheValid || stream->cachedSectorLBA != sector) {
            int status = ata_read_sectors(sector, 1, stream->cache);
            if (status != SUCCESS) return status;

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
		int sector = stream->cursor / SECTOR_SIZE;
		int offset = stream->cursor % SECTOR_SIZE;

		char cache[SECTOR_SIZE];
		int readStatus = ata_read_sectors(sector, 1, cache);
		if (readStatus != SUCCESS) return readStatus;

		uint32_t available = SECTOR_SIZE - offset;
		uint32_t toWrite = (totalRemaining < available) ? totalRemaining : available;

		memcpy(cache + offset, bufPtr, toWrite);

		ata_write_sectors(sector, 1, cache);

		// Update pointers and counters
		bufPtr += toWrite;
		stream->cursor += toWrite;
		totalRemaining -= toWrite;
	}

	return SUCCESS;
}

int stream_seek(struct Stream *stream, uint32_t sector){
	if(!stream || sector < 0){
		return INVALID_ARG;
	}

	stream->cursor = sector;

	stream->cacheValid = 0;

	return SUCCESS;
}

int stream_dispose(struct Stream *ptr){
	if(!ptr) return INVALID_ARG;

	kfree(ptr);

	return SUCCESS;
}
