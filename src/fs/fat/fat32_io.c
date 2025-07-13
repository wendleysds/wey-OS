#include <fs/fat/fat32.h>
#include <fs/fat/fatdefs.h>
#include <io/stream.h>
#include <def/status.h>
#include <def/config.h>
#include <lib/string.h>
#include <lib/utils.h>
#include <lib/mem.h>
#include <memory/kheap.h>
#include <drivers/terminal.h>
#include "fat32_internal.h"

int8_t _fat32_update(struct FAT* fat){
	if(!fat){
		return INVALID_ARG;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	int16_t status = SUCCESS;
	uint32_t fatStartSector = fat->headers.boot.rsvdSecCnt;
    uint32_t fatBystes = fat->headers.extended.FATSz32 * fat->headers.boot.bytesPerSec;

	if(_fat32_fsinfo_sig_valid(&fat->fsInfo)){
		stream_seek(stream, _SEC(fat->headers.extended.FSInfo), SEEK_SET);
		if((status = stream_write(stream, &fat->fsInfo, sizeof(fat->fsInfo))) != SUCCESS){
			status = ERROR_IO;
			goto out;
		}
	}

    stream_seek(stream, _SEC(fatStartSector), SEEK_SET);
	if((status = stream_write(stream, fat->table, fatBystes)) != SUCCESS){
		status = ERROR_IO;
	}

out:
	stream_dispose(stream);
    return status;
}

int FAT32_read(struct FAT* fat, struct FATFileDescriptor *ffd, void *buffer, uint32_t count){
	if(!fat || !ffd || !buffer || count < 0){
		return INVALID_ARG;
	}

	if (ffd->entry.DIR_Attr & ATTR_DIRECTORY)
    {
        return NOT_SUPPORTED; // Cannot read a directory
    }

	struct Stream *stream = stream_new();
	if (!stream) {
		return NO_MEMORY;
	}

    uint32_t cursor = ffd->cursor;
    uint32_t lba = _fat32_cluster_to_lba(fat, ffd->currentCluster);

    uint32_t remaining = count;
    uint32_t totalReaded = 0;

    uint32_t clusterOffset = cursor % CLUSTER_SIZE;
    uint32_t bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;

	int8_t status = SUCCESS;

    stream_seek(stream, _SEC(lba) + clusterOffset, SEEK_SET);
    while(remaining > 0){
        uint32_t toRead = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;

		if((status = stream_read(stream, (uint8_t*)buffer + totalReaded, toRead)) != SUCCESS){
			status = ERROR_IO;
			goto out;
		}

        cursor += toRead;
        totalReaded += toRead;
        remaining -= toRead;
        bytesLeftInCluster -= toRead;

        if(cursor % CLUSTER_SIZE == 0){
            uint32_t next = _fat32_next_cluster(fat, ffd->currentCluster);
            if(CHK_EOF(next)){
                status = END_OF_FILE; // Reached end of file
				goto out;
            }

            ffd->currentCluster = next;
            clusterOffset = cursor % CLUSTER_SIZE;
            bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;

            lba = _fat32_cluster_to_lba(fat, next);
            stream_seek(stream, _SEC(lba) + clusterOffset, SEEK_SET);
        }
    }

out:
	stream_dispose(stream);
    return totalReaded;
}

int FAT32_write(struct FAT *fat, struct FATFileDescriptor *ffd, const void *buffer, uint32_t size){
	if(!fat || !ffd || !buffer || size < 0){
		return INVALID_ARG;
	}

	if (ffd->entry.DIR_Attr & ATTR_DIRECTORY)
    {
        return NOT_SUPPORTED; // Cannot write to a directory
    }

	struct Stream *stream = stream_new();
	if (!stream) {
		return NO_MEMORY;
	}

    uint32_t cursor = ffd->cursor;
    uint32_t lba = _fat32_cluster_to_lba(fat, ffd->currentCluster);

    uint32_t remaining = size;
    uint32_t totalWritten = 0;

    uint32_t clusterOffset = cursor % CLUSTER_SIZE;
    uint32_t bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;

	int8_t status = SUCCESS;

    stream_seek(stream, _SEC(lba) + clusterOffset, SEEK_SET);
    while(remaining > 0){
        uint32_t toWrite = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;

		if((status = stream_write(stream, (uint8_t*)buffer + totalWritten, toWrite)) != SUCCESS){
			status = ERROR_IO;
			goto out;
		}

        cursor += toWrite;
        totalWritten += toWrite;
        remaining -= toWrite;
        bytesLeftInCluster -= toWrite;
		status = totalWritten;

        if(cursor % CLUSTER_SIZE == 0){
            uint32_t next = _fat32_next_cluster(fat, ffd->currentCluster);
            if(CHK_EOF(next)){
                next = _fat32_reserve_next_cluster(fat);
                if(next == 0xFFFFFFFF){
					status = OUT_OF_BOUNDS; // FAT Full
					goto out;
				}

                fat->table[ffd->currentCluster] = next;
                fat->table[next] = EOF;
            }

            ffd->currentCluster = next;

            clusterOffset = cursor % CLUSTER_SIZE;
            bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;

            lba = _fat32_cluster_to_lba(fat, next);
            stream_seek(stream, _SEC(lba) + clusterOffset, SEEK_SET);
        }
    }

out:
	ffd->cursor = cursor;
	if(ffd->cursor > ffd->entry.DIR_FileSize){
		ffd->entry.DIR_FileSize = ffd->cursor;
	}else{
		// TODO: Implement
		// Recalculate the file size and update EOF if necessary
	}

	stream_dispose(stream);
	if((status = _fat32_save_entry(fat, &ffd->entry, ffd->dirCluster)) != SUCCESS){
		return status;
	}

	if((status = _fat32_update(fat)) != SUCCESS){
		return status;
	}

    return totalWritten;
}
