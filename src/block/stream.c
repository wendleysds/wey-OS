#include <fs/stream.h>
#include <lib/string.h>
#include <lib/div64.h>
#include <def/err.h>

#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

struct sync_wait {
	volatile int done;
};

static void stream_endio(struct bio *bio){
	struct sync_wait *wait = bio->private;
	wait->done = 1;
}

static int stream_flush(struct Stream* stream){
	if(!stream->cacheValid || !stream->dirty){
		return SUCCESS;
	}

	struct sync_wait wait = {0};

	struct bio bio;
	bio.sector = stream->cachedSectorLBA + stream_bdev(stream)->start_sector;
	bio.buffer = stream->sector_cache;
	bio.nr_sectors = stream->sector_size;
	bio.op = BLK_WRITE;
	bio.bdev = stream_bdev(stream);
	bio.private = &wait;
	bio.end_io = stream_endio;

	blk_submit_bio(bio.bdev, &bio);

	while(!wait.done){
		// TODO: Add sleep queue
	}

	if(IS_ERR_VALUE(bio.status)){
		return bio.status;
	}

	stream->dirty = 0;
	return SUCCESS;
}

struct Stream* stream_new(struct blkdev* bdev){
	if(!bdev) return 0x0;

	struct Stream* s = (struct Stream*)kmalloc(sizeof(struct Stream));
	if(s){
		s->sector_size = bdev->disk->sec_size;
		uint8_t* cache = kmalloc(s->sector_size);
		if(!cache){
			kfree(s);
			return 0x0;
		}

		s->sector_cache = cache;
		s->pos = 0;
		s->dirty = 0;

		s->cacheValid = 0;
		s->bdev = bdev;
	}
	
	return s;
}

struct Stream* stream_dup(struct Stream* stream){
	if(!stream) return 0x0;

	struct Stream* s = (struct Stream*)kmalloc(sizeof(struct Stream));
	if(s){
		uint8_t* cache = kmalloc(stream->sector_size);
		if(!cache){
			kfree(s);
			return 0x0;
		}

		memcpy(s, stream, sizeof(struct Stream));
		s->sector_cache = cache;
		s->cacheValid = 0;
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
		sector_t sector = stream->pos;
		off_t offset = do_div(sector, stream->sector_size);

		// Check if we need to read a new sector into the cache
		// If the cache is invalid or the sector is different, read it
		// from the disk
		if (!stream->cacheValid || stream->cachedSectorLBA != sector) {
			int ret = stream_flush(stream);
			if(IS_ERR_VALUE(ret)){
				return ret;
			}

			struct bio bio;
			bio.sector = sector + stream_bdev(stream)->start_sector;
			bio.buffer = stream->sector_cache;
			bio.nr_sectors = 1;
			bio.op = BLK_READ;
			bio.bdev = stream_bdev(stream);
			bio.end_io = stream_endio;
			struct sync_wait wait = {0};
			bio.private = &wait;

			int res = blk_submit_bio(bio.bdev, &bio);
			if(IS_ERR_VALUE(res)) return res;

			while(!wait.done){
				// TODO: Add sleep queue
			}

			res = bio.status;

			if (IS_ERR_VALUE(res)) return res;

			stream->cachedSectorLBA = sector;
			stream->cacheValid = 1;
		}

		uint32_t available = stream->sector_size - offset;
		uint32_t toRead = (totalRemaining < available) ? totalRemaining : available;

		memcpy(bufPtr, stream->sector_cache + offset, toRead);

		// Update pointers and counters
		bufPtr += toRead;
		stream->pos += toRead;
		totalRemaining -= toRead;
	}

	return total - totalRemaining;
}

int stream_write(struct Stream *stream, const void *buffer, int total){
	if(!stream || !buffer || total <= 0){
		return INVALID_ARG;
	}

	int totalRemaining = total;
	const uint8_t* bufPtr = buffer;

	while(totalRemaining > 0){
		sector_t sector = stream->pos;
		off_t offset = do_div(sector, stream->sector_size);

		if (!stream->cacheValid || stream->cachedSectorLBA != sector){
			int ret = stream_flush(stream);
			if(IS_ERR_VALUE(ret)){
				return ret;
			}

			struct sync_wait wait = {0};
			struct bio bio;

			bio.sector = sector + stream_bdev(stream)->start_sector;
			bio.buffer = stream->sector_cache;
			bio.nr_sectors = stream->sector_size;
			bio.op = BLK_READ;
			bio.bdev = stream_bdev(stream);
			bio.private = &wait;
			bio.end_io = stream_endio;

			blk_submit_bio(bio.bdev, &bio);

			while(!wait.done){
				// TODO: Add sleep queue
			}

			if(IS_ERR_VALUE(bio.status)){
				return bio.status;
			}

			stream->cachedSectorLBA = sector;
			stream->cacheValid = 1;
		}

		uint32_t available = stream->sector_size - offset;
		uint32_t toWrite = (totalRemaining < available) ? totalRemaining : available;

		memcpy(stream->sector_cache + offset, bufPtr, toWrite);

		stream->dirty = 1;  // <--- aqui está o coração

		bufPtr += toWrite;
		stream->pos += toWrite;
		totalRemaining -= toWrite;
	}

	return total - totalRemaining;
}

off_t stream_seek(struct Stream *stream, off_t offset, uint8_t whence){
	if(!stream){
		return NULL_PTR;
	}

	uint64_t size = (
		stream->bdev->nr_sectors *
		stream->bdev->disk->sec_size
	);

	off_t target;
	switch(whence){
		case SEEK_SET:
			target = offset;
			break;
		case SEEK_CUR:
			target = stream->pos + offset;
			break;
		case SEEK_END:
			target = size = offset;
			break;
		default: return INVALID_ARG;
	}

	if(target > size){
		return OVERFLOW;
	}

	stream->pos = target;
	return size - target;
}

int stream_dispose(struct Stream *ptr){
	if(!ptr) return INVALID_ARG;

	int res = stream_flush(ptr);
	if(IS_ERR_VALUE(res)){
		return res;
	}

	kfree(ptr->sector_cache);
	kfree(ptr);

	return res;
}
