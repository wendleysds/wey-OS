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

int64_t _fat32_get_entry_lba(struct FAT* fat, struct FAT32DirectoryEntry* entry, uint32_t dirCluster){
	if(!entry || dirCluster < 2){
		return INVALID_ARG;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	int8_t status = FILE_NOT_FOUND;
	uint32_t cluster = dirCluster;
	while (!CHK_EOF(dirCluster)) {
		uint32_t lba = _fat32_cluster_to_lba(fat, cluster);
		stream_seek(stream, _SEC(lba), SEEK_SET);

		for (uint32_t i = 0; i < (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
			struct FAT32DirectoryEntry buffer;
			if (stream_read(stream, &buffer, sizeof(buffer)) != SUCCESS) {
				status = ERROR_IO;
				goto out;
			}

			if (buffer.DIR_Name[0] == 0x0) {
				status = FILE_NOT_FOUND;
				goto out;
			}

			if (buffer.DIR_Name[0] == 0xE5) {
				continue;
			}

			if(memcmp(buffer.DIR_Name, entry->DIR_Name, 11) == 0){
				stream_dispose(stream);
				return lba + (i * sizeof(buffer));
			}
		}

		cluster = _fat32_next_cluster(fat, cluster);
	}

out:
	stream_dispose(stream);
	return status;
}

int8_t _fat32_free_chain(struct FAT* fat, uint32_t start){
	if(!fat || start < 2 || start > fat->totalClusters){
		return INVALID_ARG;
	}

	int count = 0;
	uint32_t cur = start;
	while(1){
		if(!((cur <= 0) < fat->totalClusters)){
			break;
		}

		uint32_t next = _fat32_next_cluster(fat, cur);
		fat->table[cur] = FAT_UNUSED;
		count++;

		if(CHK_EOF(next) || next == FAT_UNUSED){
			break;
		}

		cur = next;
		
		if(count > fat->totalClusters){
			break;
		}
	}

	if(fat->fsInfo.freeClusterCount != 0xFFFFFFFF){
		fat->fsInfo.freeClusterCount += count;
	}

	fat->fsInfo.nextFreeCluster = start;

	return _fat32_update(fat);
}

int8_t _fat32_truncate_entry(struct FAT* fat, struct FAT32DirectoryEntry* entry){
	if(!fat || !entry){
		return INVALID_ARG;
	}

	int8_t status = SUCCESS;
	uint32_t s = _CLUSTER(entry->DIR_FstClusHI, entry->DIR_FstClusLO);
	uint32_t n = _fat32_next_cluster(fat, s);

	entry->DIR_FileSize = 0;
	if((status = _fat32_free_chain(fat, s)) != SUCCESS){
		return status;
	}

	if(_fat32_fsinfo_sig_valid(&fat->fsInfo)){
		if(!CHK_EOF(n)){
			fat->fsInfo.nextFreeCluster = n;
		}

		if(fat->fsInfo.freeClusterCount != 0xFFFFFFFF){
			fat->fsInfo.freeClusterCount--;
		}
	}

    fat->table[n] = EOF;

	return _fat32_update(fat);
}

int FAT32_stat(struct FAT* fat, const char* restrict pathname, struct Stat* restrict statbuf){
	if(!fat || !statbuf || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	int status = SUCCESS;
	struct FAT32DirectoryEntry entry;
	if((status = _fat32_traverse_path(fat, pathname, &entry)) != SUCCESS){
		return status;
	}

	statbuf->fileSize = entry.DIR_FileSize;
	statbuf->attr = entry.DIR_Attr;
	statbuf->creDate = entry.DIR_CrtDate;
	statbuf->modDate = entry.DIR_WrtTime; 

	return status;
}

int FAT32_seek(struct FAT* fat, struct FATFileDescriptor* ffd, uint32_t offset, uint8_t whence){
	if(!fat || !ffd){
		return INVALID_ARG;
	}

	uint32_t target;
	switch(whence){
		case SEEK_SET:
			target = offset	;	
			break;
		case SEEK_CUR:
			target = ffd->cursor + offset;
			break;
		case SEEK_END:
			if(offset > ffd->entry.DIR_FileSize){
				return INVALID_ARG; // Cannot seek beyond file size
			}

			target = ffd->entry.DIR_FileSize - offset;
			break;
		default:
			return INVALID_ARG;
	}

	if(target > ffd->entry.DIR_FileSize){
		return INVALID_ARG; // Cannot seek beyond file size
	}

	uint32_t clusterOffset = target / CLUSTER_SIZE;

	uint32_t cluster = ffd->firstCluster;
	for(uint32_t i = 0; i < clusterOffset; i++){
		cluster= _fat32_next_cluster(fat, cluster);
		if(CHK_EOF(cluster)) {
			return END_OF_FILE; // Reached end of file
		}
	}

	ffd->currentCluster = cluster;
	ffd->cursor = target;

	return SUCCESS;
}
