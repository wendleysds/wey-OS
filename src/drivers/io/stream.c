#include <io/stream.h>
#include <io/ata.h>
#include <memory/kheap.h>
#include <lib/mem.h>

#include <def/status.h>

struct Stream* stream_new(){
	struct Stream* s = (struct Stream*)kmalloc(sizeof(struct Stream));
	if(!s) return 0x0;
	
	s->unused = 0;
	s->cacheDirt = 0;
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
		int sector = stream->unused / SECTOR_SIZE;
		int offset = stream->unused % SECTOR_SIZE;

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
		stream->unused += toRead;
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
		int sector = stream->unused / SECTOR_SIZE;
		int offset = stream->unused % SECTOR_SIZE;

		// Check if we need to write to a new sector
		// If the cache is invalid or the sector is different, read it
		// from the disk
		// Note: This is a write operation, so we need to ensure the cache is
		// up-to-date before writing to it.
		if (!stream->cacheValid || stream->cachedSectorLBA != sector) {
			int flushStatus = stream_flush(stream);
			if (flushStatus != SUCCESS) return flushStatus;

			int readStatus = ata_read_sectors(sector, 1, stream->cache);
			if (readStatus != SUCCESS) return readStatus;

			stream->cachedSectorLBA = sector;
			stream->cacheValid = 1;
		}

		uint32_t available = SECTOR_SIZE - offset;
		uint32_t toWrite = (totalRemaining < available) ? totalRemaining : available;

		memcpy(stream->cache + offset, bufPtr, toWrite);

		// Update pointers and counters
		bufPtr += toWrite;
		stream->unused += toWrite;
		totalRemaining -= toWrite;

		stream->cacheDirt = 1; // Mark cache as dirty
	}

	return NOT_IMPLEMENTED;
}

int stream_seek(struct Stream *stream, uint32_t sector){
	if(!stream || sector < 0){
		return INVALID_ARG;
	}

	stream->unused = sector;
	stream->cacheValid = 0;

	return SUCCESS;
}

int stream_dispose(struct Stream *ptr){
	if(!ptr) return INVALID_ARG;

	if(ptr->cacheDirt){
		stream_flush(ptr); // Ensure any dirty cache is flushed before disposal
	}

	kfree(ptr);

	return SUCCESS;
}

int stream_flush(struct Stream *stream){
	if(!stream) return INVALID_ARG;

	if(!stream->cacheDirt || !stream->cacheValid){
		return SUCCESS; // Nothing to flush
	}

	int status = ata_write_sectors(stream->cachedSectorLBA, 1, stream->cache);
	if(status != SUCCESS){
		return status; // Error writing to disk
	}

	stream->cacheDirt = 0;
	stream->cacheValid = 0;
	return SUCCESS;
}
