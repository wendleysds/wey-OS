#include <io/stream.h>
#include <io/ata.h>
#include <memory/kheap.h>
#include <lib/mem.h>

#include <def/status.h>

#define SECTOR_SIZE 512

struct Stream* stream_new(){
	struct Stream* s = (struct Stream*)kmalloc(sizeof(struct Stream));
	if(!s) return 0x0;
	
	s->unused = 0;

	return s;
}

int stream_read(struct Stream *stream, void *buffer, int total){
	int status = SUCCESS;
	int totalRemaining = total;
	char* bufPtr = (char*)buffer;

	while(totalRemaining > 0){
		int sector = stream->unused / SECTOR_SIZE;
		int offset = stream->unused % SECTOR_SIZE;

		int bytesThisSectorCanRead = SECTOR_SIZE - offset;
		if(bytesThisSectorCanRead > totalRemaining)
			bytesThisSectorCanRead = totalRemaining;

		char sectorBuff[SECTOR_SIZE];
		status = ata_read_sectors(sector, 1, sectorBuff);
		if(status != SUCCESS)
			break;

		memcpy(bufPtr, sectorBuff + offset, bytesThisSectorCanRead);

		// Update pointers and counters
		bufPtr += bytesThisSectorCanRead;
		stream->unused += bytesThisSectorCanRead;
		totalRemaining -= bytesThisSectorCanRead;
	}

	return status;
}

void stream_seek(struct Stream *stream, uint32_t sector){
	stream->unused = sector;
}

void stream_dispose(struct Stream *ptr){
	kfree(ptr);
}

