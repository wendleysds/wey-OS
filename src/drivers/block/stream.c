#include <io/stream.h>
#include <mm/kheap.h>
#include <lib/string.h>
#include <def/status.h>

struct Stream* stream_new(struct blkdev* bdev){
	if(!bdev) return 0x0;

	struct Stream* s = (struct Stream*)kzalloc(sizeof(struct Stream));
	if(s){
		s->cacheValid = 0;
		s->bdev = bdev;

		s->fbdev.f_op = (struct file_operations*)bdev->ops;
		s->fbdev.private_data = bdev->dev->driver_data;
	}
	
	return s;
}

struct Stream* stream_dup(struct Stream* stream){
	if(!stream) return 0x0;

	struct Stream* s = (struct Stream*)kmalloc(sizeof(struct Stream));
	if(s){
		memcpy(s, stream, sizeof(struct Stream));
	}

	return s;
}

int stream_read(struct Stream *stream, void *buffer, int total){
	if(!stream || !buffer || total <= 0){
		return INVALID_ARG;
	}

	int totalRemaining = total;
	uint8_t* bufPtr = (uint8_t*)buffer;

	while(totalRemaining > 0){
		uint32_t sector = stream->fbdev.pos / SECTOR_SIZE;
		uint64_t offset = stream->fbdev.pos % SECTOR_SIZE;

		// Check if we need to read a new sector into the cache
		// If the cache is invalid or the sector is different, read it
		// from the disk
		if (!stream->cacheValid || stream->cachedSectorLBA != sector) {
			int status = stream->bdev->ops->read(
				&stream->fbdev, stream->cache,
				sizeof(stream->cache)
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
		stream->fbdev.pos += toRead;
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
		uint64_t offset = stream->fbdev.pos % SECTOR_SIZE;

		char cache[SECTOR_SIZE];
		int readStatus = stream->bdev->ops->read(
				&stream->fbdev, stream->cache,
				sizeof(stream->cache)
		);

		if (readStatus < 0) return readStatus;

		uint32_t available = SECTOR_SIZE - offset;
		uint32_t toWrite = (totalRemaining < available) ? totalRemaining : available;

		memcpy(cache + offset, bufPtr, toWrite);

		int writeStatus = stream->bdev->ops->write(
			&stream->fbdev, stream->cache,
			sizeof(stream->cache)
		);

		if(writeStatus < 0) return writeStatus;

		// Update pointers and counters
		bufPtr += toWrite;
		stream->fbdev.pos += toWrite;
		totalRemaining -= toWrite;
	}

	return SUCCESS;
}

int stream_seek(struct Stream *stream, uint32_t offset, uint8_t whence){
	if(!stream || offset < 0){
		return INVALID_ARG;
	}

	if(!stream->bdev->ops->lseek){
		return NOT_SUPPORTED;
	}

	int res = stream->bdev->ops->lseek(&stream->fbdev, offset, whence);

	return res;
}

int stream_dispose(struct Stream *ptr){
	if(!ptr) return INVALID_ARG;

	kfree(ptr);

	return SUCCESS;
}
