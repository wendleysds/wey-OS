#include <fs/fat/fat32.h>
#include <fs/fat/fatdefs.h>
#include <io/stream.h>
#include <def/status.h>
#include <def/config.h>
#include <lib/string.h>
#include <lib/utils.h>
#include <lib/mem.h>
#include <memory/kheap.h>
#include "fat32_internal.h"

static int8_t _find_entry(struct FAT *fat, const char *itemName, uint32_t dirCluster, struct FAT32DirectoryEntry *buff) {
	if (!fat || !itemName || !buff) {
		return INVALID_ARG;
	}

	struct Stream *stream = stream_new();
	if (!stream) {
		return NO_MEMORY;
	}

	char filename[12];
	format_fat_name(itemName, filename);

	uint32_t cluster = dirCluster;
	int status = FILE_NOT_FOUND;

	while (!CHK_EOF(cluster)) {
		uint32_t lba = _fat32_cluster_to_lba(fat, cluster);
		stream_seek(stream, _SEC(lba), SEEK_SET);

		for (uint32_t i = 0; i < (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
			struct FAT32DirectoryEntry entry;
			if (stream_read(stream, &entry, sizeof(entry)) != SUCCESS) {
				status = ERROR_IO;
				goto out;
			}

			if (entry.DIR_Name[0] == 0x0) {
				status = FILE_NOT_FOUND;
				goto out;
			}

			if (entry.DIR_Name[0] == 0xE5) {
				continue;
			}

			if (memcmp((char *)entry.DIR_Name, filename, 11) == 0) {
				memcpy(buff, &entry, sizeof(entry));
				status = SUCCESS;
				goto out;
			}
		}

		cluster = _fat32_next_cluster(fat, cluster);
	}

out:
	stream_dispose(stream);
	return status;
}

int8_t _fat32_save_entry(struct FAT* fat, struct FAT32DirectoryEntry* entry, uint32_t dirCluster){
	if(!entry || dirCluster < 2){
		return INVALID_ARG;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	int8_t status = SUCCESS;
	int lba = _fat32_get_entry_lba(fat, entry, dirCluster);
	if(lba < 0){
		status = lba;
		goto out;
	}

	stream_seek(stream, _SEC(lba), SEEK_SET);
	if (stream_write(stream, entry, sizeof(struct FAT32DirectoryEntry)) != SUCCESS) {
		status = ERROR_IO;
		goto out;
	}

out:
	stream_dispose(stream);
	return status;
}

int8_t _fat32_create_entry(struct FAT *fat, const char *pathname, int attr){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	char pathCopy[PATH_MAX];
	memset(pathCopy, 0x0, sizeof(pathCopy));
	strncpy(pathCopy, pathname, PATH_MAX - 1);

	char *slash = strrchr(pathCopy, '/');
	char *filename = slash + 1;
	*slash = '\0';

	if (strlen(filename) > 11){
		return NOT_SUPPORTED;
	}

	if(filename[0] == '.'){
		return INVALID_NAME;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	int8_t status = SUCCESS;
	uint32_t dirCluster = fat->rootClus;

	struct FAT32DirectoryEntry buffer;
	memset(&buffer, 0, sizeof(buffer));

	if(_fat32_traverse_path(fat, pathname, &buffer) == SUCCESS){
		status = FILE_EXISTS;
		goto out;
	}

	if(strlen(pathCopy) > 0){
		if ((status =  _fat32_traverse_path(fat, pathCopy, &buffer)) != SUCCESS){
			goto out;
		}

		if (buffer.DIR_Attr & ATTR_ARCHIVE){
			status = INVALID_ARG;
			goto out;
		}

		dirCluster = _CLUSTER(buffer.DIR_FstClusHI, buffer.DIR_FstClusLO);
	}

	uint32_t cluster = dirCluster;
	while(1) {
		uint32_t lba = _fat32_cluster_to_lba(fat, cluster);
		stream_seek(stream, _SEC(lba), SEEK_SET);

		for(uint32_t i = 0; i < (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
			if(stream_read(stream, &buffer, sizeof(buffer)) != SUCCESS) {
				status = ERROR_IO;
				goto out;
			}

			if(buffer.DIR_Name[0] != 0x0 && buffer.DIR_Name[0] != 0xE5){
				continue;
			}

			uint32_t _cluster = _fat32_reserve_next_cluster(fat);
			fat->table[_cluster] = EOF;

			char name[12];
			format_fat_name(filename, name);

			memset(&buffer, 0x0, sizeof(buffer));
			memcpy(buffer.DIR_Name, name, 11);
			buffer.DIR_Attr = attr;
			buffer.DIR_FstClusHI = _cluster >> 16;
			buffer.DIR_FstClusLO = _cluster;

			stream_seek(stream, -sizeof(buffer), SEEK_CUR);
			if(stream_write(stream, &buffer, sizeof(buffer)) != SUCCESS) {
				status = ERROR_IO;
				goto out;
			}

			status = _fat32_update(fat);
			goto out;
		}

		cluster = _fat32_next_cluster(fat, cluster);

		// Reserve a new cluster for the directory if necessary.
		if(CHK_EOF((cluster))){
			uint32_t new = _fat32_reserve_next_cluster(fat);
			if(new == 0xFFFFFFFF){
				status = OUT_OF_BOUNDS; // FAT Full
				goto out;
			}

			fat->table[cluster] = new;
			fat->table[new] = EOF;
			cluster = new;
		}
	}

out:
	stream_dispose(stream);
	return status;
}

int8_t _fat32_remove_entry(struct FAT *fat, const char *pathname){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	char dirpath[PATH_MAX];
	memset(dirpath, 0x0, sizeof(dirpath));
	strncpy(dirpath, pathname, PATH_MAX - 1);

	char *slash = strrchr(dirpath, '/');
	char *filename = slash + 1;
	*slash = '\0';

	if (strlen(filename) > 11){
		return NOT_SUPPORTED;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	int8_t status = SUCCESS;
	uint32_t dirCluster = fat->rootClus;
	struct FAT32DirectoryEntry buffer;
	if(strlen(dirpath) > 0){
		if ((status =  _fat32_traverse_path(fat, dirpath, &buffer)) != SUCCESS){
			return status;
		}

		if (buffer.DIR_Attr & ATTR_ARCHIVE){
			return INVALID_ARG;
		}

		dirCluster = _CLUSTER(buffer.DIR_FstClusHI, buffer.DIR_FstClusLO);
	}

	if ((status = _fat32_traverse_path(fat, pathname, &buffer)) != SUCCESS)
	{
		stream_dispose(stream);
		return status;
	}

	_fat32_free_chain(fat, _CLUSTER(buffer.DIR_FstClusHI, buffer.DIR_FstClusLO));
	int lba = _fat32_get_entry_lba(fat, &buffer, dirCluster);
	if(lba < 0){
		stream_dispose(stream);
		return lba;
	}

	buffer.DIR_Name[0] = 0xE5;
	stream_seek(stream, _SEC(lba), SEEK_SET);
	if(stream_write(stream, &buffer, sizeof(buffer)) != SUCCESS){
		stream_dispose(stream);
		return ERROR_IO;
	}

	fat->fsInfo.nextFreeCluster = _CLUSTER(buffer.DIR_FstClusHI, buffer.DIR_FstClusLO);

	stream_dispose(stream);
	return _fat32_update(fat);
}

int8_t _fat32_traverse_path(struct FAT *fat, const char *path, struct FAT32DirectoryEntry *buff){
	if(!fat || !buff || !path || strlen(path) > PATH_MAX){
		return INVALID_ARG;
	}

	char pathCopy[PATH_MAX];
	memset(pathCopy, 0x0, sizeof(pathCopy));
	strncpy(pathCopy, path, PATH_MAX - 1);

	char *token = strtok(pathCopy, "/");
	if (!token){
		return INVALID_ARG;
	}

	int8_t status = SUCCESS;
	if ((status = _find_entry(fat, token, fat->rootClus, buff)) != SUCCESS){
		return status;
	}

	while ((token = strtok(NULL, "/"))){
		if(buff->DIR_Attr & ATTR_ARCHIVE){
			return NOT_SUPPORTED;
		}

		if((status = _find_entry(fat, token, _CLUSTER(buff->DIR_FstClusHI, buff->DIR_FstClusLO), buff)) != SUCCESS){
			return status;
		}
	}

	return status;
}