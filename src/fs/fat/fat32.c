#include <fs/fat/fat32.h>
#include <fs/fat/fatdefs.h>
#include <fs/fs.h>
#include <def/status.h>
#include <def/config.h>
#include <lib/string.h>
#include <lib/mem.h>
#include <io/stream.h>
#include <memory/kheap.h>
#include "fat32_internal.h"

int FAT32_init(struct FAT* fat){
	if(!fat){
		return INVALID_ARG;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	int8_t status;

	stream_seek(stream, 0, SEEK_SET);
	if((status = stream_read(stream, &fat->headers, sizeof(fat->headers))) != SUCCESS){
		goto err;
	}

	stream_seek(stream, _SEC(fat->headers.extended.FSInfo), SEEK_SET);
	if((status = stream_read(stream, &fat->fsInfo,  sizeof(fat->fsInfo))) != SUCCESS){
		goto err;
	}

	uint32_t fatStartSector = fat->headers.boot.rsvdSecCnt;
    uint32_t fatSize = fat->headers.extended.FATSz32;
    uint32_t fatTotalClusters = fatSize * fat->headers.boot.bytesPerSec / 4;
    uint32_t fatBystes = fatSize * fat->headers.boot.bytesPerSec;

    if (!_fat32_fsinfo_sig_valid(&fat->fsInfo)){
        status = INVALID_FS; // This driver depends the FSInfo to work
		goto err;
    }

    uint32_t *table = (uint32_t *)kmalloc(fatBystes);
    if (!table)
    {
        status = NO_MEMORY;
        goto err;
    }

	stream_seek(stream, _SEC(fatStartSector), SEEK_SET);
	if((status = stream_read(stream, table, fatBystes)) != SUCCESS){
		goto err;
	}

    fat->firstDataSector = fatStartSector + (fat->headers.boot.numFATs * fatSize);
    fat->totalClusters = fatTotalClusters;
    fat->table = table;
    fat->rootClus = fat->headers.extended.rootClus;

err:
	stream_dispose(stream);
	return status;
}

int FAT32_open(struct FAT* fat, void** outFDPtr, const char *pathname, uint8_t flags){
	if(!fat || !outFDPtr || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	struct FAT32DirectoryEntry buffer;

	char dirname[PATH_MAX];
	memset(dirname, 0x0, sizeof(dirname));
	strncpy(dirname, pathname, PATH_MAX - 1);

	char *slash = strrchr(dirname, '/');
	*slash = '\0';

	int8_t status = SUCCESS;
	uint32_t dirCluster = fat->rootClus;
	if(strlen(dirname) > 0){
		if ((status =  _fat32_traverse_path(fat, dirname, &buffer)) != SUCCESS){
			return status;
		}

		if (buffer.DIR_Attr & ATTR_ARCHIVE){
			return INVALID_ARG;
		}

		dirCluster = _CLUSTER(buffer.DIR_FstClusHI, buffer.DIR_FstClusLO);
	}

	status = _fat32_traverse_path(fat, pathname, &buffer);
	if(status == FILE_NOT_FOUND && (flags & O_CREAT)){
		if((status = FAT32_create(fat, pathname)) != SUCCESS){
			return status;
		}

		status = _fat32_traverse_path(fat, pathname, &buffer);
	}

	if(status != SUCCESS){
		return status;
	}

	if(flags & O_TRUNC){
		if(flags & O_WRONLY){
			if((status = _fat32_truncate_entry(fat, &buffer)) != SUCCESS){
				return status;
			}
		}
	}

	struct FATFileDescriptor* ffd = (struct FATFileDescriptor*)kcalloc(sizeof(struct FATFileDescriptor));
	if(!ffd){
		return NO_MEMORY;
	}

	ffd->firstCluster = _CLUSTER(buffer.DIR_FstClusHI, buffer.DIR_FstClusLO);
	ffd->currentCluster = ffd->firstCluster;
	ffd->dirCluster = dirCluster;
	ffd->entry = buffer;
	*outFDPtr = (void*)ffd;

	return SUCCESS;
}

int FAT32_close(struct FATFileDescriptor *ffd){
	if(!ffd){
		return NULL_PTR;
	}

	kfree(ffd);

	return SUCCESS;
}

int FAT32_mkdir(struct FAT* fat, const char* pathname){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	if(strlen(pathname) == 0){
		return INVALID_ARG;
	}

	if(strbrk(pathname, ILEGAL_CHARS)){
		return INVALID_NAME;
	}

	// TODO: Implement do add '.' and '..' dirs to the new (pathname) directory

	return _fat32_create_entry(fat, pathname, ATTR_DIRECTORY);
}

int FAT32_rmdir(struct FAT* fat, const char* pathname){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	int8_t status;
	struct FAT32DirectoryEntry entry;
	if((status = _fat32_traverse_path(fat, pathname, &entry)) != SUCCESS){
		return status;
	}

	if(entry.DIR_Attr & ATTR_ARCHIVE){
		return NOT_SUPPORTED;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	uint32_t entryCount = 0;
	uint32_t cluster = _CLUSTER(entry.DIR_FstClusHI, entry.DIR_FstClusLO) ;
	int8_t result = SUCCESS;

	while (!CHK_EOF(cluster)) {
		uint32_t lba = _fat32_cluster_to_lba(fat, cluster);
		stream_seek(stream, _SEC(lba), SEEK_SET);

		for (uint32_t i = 0; i < (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
			struct FAT32DirectoryEntry dirEntry;
			if (stream_read(stream, &dirEntry, sizeof(dirEntry)) != SUCCESS) {
				result = ERROR_IO;
				goto out;
			}

			if (dirEntry.DIR_Name[0] == 0x0) {
				// End of directory entries
				goto check_empty;
			}

			if (dirEntry.DIR_Name[0] == 0xE5 || dirEntry.DIR_Name[0] == '.') {
				continue;
			}

			entryCount++;
			if (entryCount > 2) {
				result = DIR_NOT_EMPTY;
				goto out;
			}
		}

		cluster = _fat32_next_cluster(fat, cluster);
	}

check_empty:
	if (entryCount > 2) {
		result = DIR_NOT_EMPTY;
	} else {
		result = _fat32_remove_entry(fat, pathname);
	}

out:
	stream_dispose(stream);
	return result;
}

int FAT32_create(struct FAT* fat, const char* pathname){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	if(strlen(pathname) == 0){
		return INVALID_ARG;
	}

	if(strbrk(pathname, ILEGAL_CHARS)){
		return INVALID_NAME;
	}

	return _fat32_create_entry(fat, pathname, ATTR_ARCHIVE);
}

int FAT32_remove(struct FAT* fat, const char* pathname){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	int8_t status;
	struct FAT32DirectoryEntry entry;
	if((status = _fat32_traverse_path(fat, pathname, &entry)) != SUCCESS){
		return status;
	}

	if(entry.DIR_Attr & ATTR_DIRECTORY){
		return NOT_SUPPORTED; // Cannot remove a directory
	}

	return _fat32_remove_entry(fat, pathname); // Remove a file or directory
}
