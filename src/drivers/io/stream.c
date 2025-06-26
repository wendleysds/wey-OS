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

int stream_write(struct Stream *stream, void *buffer, int total){
	if(!stream || !buffer){
		return INVALID_ARG;
	}
	
	return NOT_IMPLEMENTED;
}

void stream_seek(struct Stream *stream, uint32_t sector){
	stream->unused = sector;
	stream->cacheValid = 0;
}

void stream_dispose(struct Stream *ptr){
	kfree(ptr);
}

