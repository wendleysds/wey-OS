#include "vfat_fs_internal.h"
#include <def/status.h>

static int fat_entry_update(struct FAT* fat, uint32_t dirCluster, struct FATLegacyEntry* entry){
    if(!entry || dirCluster < 2){
        return INVALID_ARG;
    }

    struct Stream* stream = stream_new(fat->bdev);
	if(!stream){
		return NO_MEMORY;
	}

    off_t offset = fat_get_entry_offset(fat, dirCluster, entry);
    if(offset < 0){
		stream_dispose(stream);
        return offset;
    }

    stream_seek(stream, offset, SEEK_SET);
    if (stream_write(stream, entry, sizeof(struct FATLegacyEntry)) < 0) {
		stream_dispose(stream);
        return ERROR_IO;
    }

	stream_dispose(stream);
    return SUCCESS;
}

int fat_read(struct file *file, void *buffer, uint32_t count){
    if(!file || !buffer || count == 0){
        return INVALID_ARG;
    }

    struct FATFileDescriptor* fd = (struct FATFileDescriptor*)file->inode->private_data;
    struct FAT* fat = fd->fat;

    if (fd->entry.attr & ATTR_DIRECTORY){
        return NOT_SUPPORTED; // Cannot write to a directory
    }

    uint32_t cursor = file->pos;
    uint32_t lba = fat_cluster_to_lba(fat, fd->currentCluster);

    uint32_t remaining = count;

	if(remaining > fd->entry.fileSize - cursor){
		remaining = fd->entry.fileSize - cursor;
		if(remaining == 0){
			return END_OF_FILE; // Reached end of file
		}
	}

    struct Stream* stream = stream_new(fat->bdev);
	if(!stream){
		return NO_MEMORY;
	}

    uint32_t totalReaded = 0;

    uint32_t clusterOffset = cursor % fd->fat->clusterSize;
    uint32_t bytesLeftInCluster = fat->clusterSize - clusterOffset;

    stream_seek_lba(stream, lba, SEEK_SET);
	stream_seek(stream, clusterOffset, SEEK_CUR);
    while(remaining > 0){
        uint32_t toRead = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;

        if(stream_read(stream, (uint8_t*)buffer + totalReaded, toRead) < 0){
			stream_dispose(stream);
            return ERROR_IO;
        }

        cursor += toRead;
        totalReaded += toRead;
        remaining -= toRead;
        bytesLeftInCluster -= toRead;

        if(cursor % fat->clusterSize == 0){
            uint32_t next = fat_next_cluster(fat, fd->currentCluster);
            if(fat_is_eof(fat, next)){
                file->pos = fd->entry.fileSize;
				stream_dispose(stream);
                return END_OF_FILE; // Reached end of file
            }

            fd->currentCluster = next;
            clusterOffset = cursor % fat->clusterSize;
            bytesLeftInCluster = fat->clusterSize - clusterOffset;

            lba = fat_cluster_to_lba(fat, next);
            stream_seek_lba(stream, lba, SEEK_SET);
        }
    }

	stream_dispose(stream);
    file->pos = cursor;
    return totalReaded;
}

int fat_write(struct file *file, const void *buffer, uint32_t count){
    if(!file || !buffer || count < 0){
        return INVALID_ARG;
    }

    struct FATFileDescriptor* fd = (struct FATFileDescriptor*)file->inode->private_data;
    struct FAT* fat = fd->fat;

    if (fd->entry.attr & ATTR_DIRECTORY){
        return NOT_SUPPORTED; // Cannot write to a directory
    }

    if(fd->entry.attr & ATTR_READ_ONLY){
        return WRITE_FAIL;
    }

    uint32_t cursor = file->pos;
    uint32_t lba = fat_cluster_to_lba(fat, fd->currentCluster);

    uint32_t remaining = count;
    uint32_t totalWritten = 0;

    uint32_t clusterOffset = cursor % fd->fat->clusterSize;
    uint32_t bytesLeftInCluster = fat->clusterSize - clusterOffset;

    struct Stream* stream = stream_new(fat->bdev);
	if(!stream){
		return NO_MEMORY;
	}

    int8_t status;
    stream_seek_lba(stream, lba, SEEK_SET);
	stream_seek(stream, clusterOffset, SEEK_CUR);
    while(remaining > 0){
        uint32_t toWrite = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;
        int res;
        if((res = stream_write(stream, (uint8_t*)buffer + totalWritten, toWrite)) < 0){
            status = ERROR_IO;
            goto out;
        }

        cursor += toWrite;
        totalWritten += toWrite;
        remaining -= toWrite;
        bytesLeftInCluster -= toWrite;

        if(cursor % fat->clusterSize == 0){
            uint32_t next = fat_next_cluster(fat, fd->currentCluster);

            if(fat_is_eof(fat, next)){
                next = fat_append_cluster(fat, fd->currentCluster);

                if(next == FAT_INVALID){
                    status = OUT_OF_BOUNDS;
                    goto out;
                }
            }

            fd->currentCluster = next;

            clusterOffset = cursor % fat->clusterSize;
            bytesLeftInCluster = fat->clusterSize - clusterOffset;

            lba = fat_cluster_to_lba(fat, next);
            stream_seek_lba(stream, lba, SEEK_SET);
        }
    }

out:
	stream_dispose(stream);

    file->pos = cursor;
    if(file->pos > fd->entry.fileSize){
        fd->entry.fileSize = file->pos;
    }else{
        // TODO: Implement
        // Recalculate the file size and update EOF if necessary
    }

    if((status = fat_entry_update(fat, fd->parentDirCluster, &fd->entry)) != SUCCESS){
        return status;
    }

    if((status = fat_update(fat)) != SUCCESS){
        return status;
    }

    return totalWritten;
}

off_t fat_lseek(struct file *file, off_t offset, int whence){
    if(!file){
        return INVALID_ARG;
    }

    struct FATFileDescriptor* fd = (struct FATFileDescriptor*)file->inode->private_data;
    uint32_t filesize = file->inode->size;

    uint32_t target;
    switch(whence){
        case SEEK_SET:
            target = offset;	
            break;
        case SEEK_CUR:
            target = file->pos + offset;
            break;
        case SEEK_END:
            target = filesize + offset;
            break;
        default:
            return INVALID_ARG;
    }

    if(target > filesize){
        return OVERFLOW;
    }

    uint32_t clusterOffset = target / fd->fat->clusterSize;

    uint32_t cluster = fd->firstCluster;
    for(uint32_t i = 0; i < clusterOffset; i++){
        cluster = fat_next_cluster(fd->fat, cluster);
        if(fat_is_eof(fd->fat, cluster)) {
            return END_OF_FILE; // Reached end of file
        }
    }

    fd->currentCluster = cluster;
    file->pos = target;

    return filesize - target;
}

int fat_close(struct file *file){
    if(!file){
        return INVALID_ARG;
    }

    return OK; // No specific close operation needed for FAT files
}

int fat_update(struct FAT* fat){
    if(!fat){
        return INVALID_ARG;
    }

    struct Stream* stream = stream_new(fat->bdev);
	if(!stream){
		return NO_MEMORY;
	}

    int8_t status = SUCCESS;

    uint32_t fatStartSector = fat->headers.boot.rsvdSecCnt;
    uint32_t fatSize = fat->type == FAT_TYPE_32 ? 
        fat->headers.extended.fat32.FATSz32 : fat->headers.boot.FATSz16;

    uint32_t fatBytes = fatSize * fat->headers.boot.bytesPerSec;

    void* table;

    switch (fat->type)
    {
        case FAT_TYPE_12: table = fat->table.fat12; break;
        case FAT_TYPE_16: table = fat->table.fat16; break;
        case FAT_TYPE_32: table = fat->table.fat32; break;
        default:
			status = INVALID_ARG;
			goto out;
    }

    stream_seek_lba(stream, fatStartSector, SEEK_SET);
    if ((status = stream_write(stream, table, fatBytes)) < 0) {
        status = ERROR_IO;
        goto out;
    }

    if(fat32_fsinfo_sig_valid(&fat->fsInfo) && fat->fsInfo.nextFreeCluster != -1){
        stream_seek_lba(stream, fat->headers.extended.fat32.FSInfo, SEEK_SET);
        if((status = stream_write(stream, &fat->fsInfo, sizeof(fat->fsInfo))) < 0){
            status = ERROR_IO;
            goto out;
        }
    }

out:
	stream_dispose(stream);
    return status;
}
