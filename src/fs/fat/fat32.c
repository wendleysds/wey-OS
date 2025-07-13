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

#define CLUSTER_SIZE 4096
#define EOF 0x0FFFFFF8

#define ILEGAL_CHARS "+,;=[]"

#define CHK_EOF(cluster) \
	(cluster >= EOF)

#define _SEC(lba) \
    (lba * SECTOR_SIZE)

static uint8_t _fsinfo_sig_valid(struct FAT32FSInfo* fsInfo) {
	return (fsInfo->leadSignature == 0x41615252 &&
        	fsInfo->structSignature == 0x61417272 &&
        	fsInfo->trailSignature == 0xAA550000);
}

static uint32_t _cluster_to_lba(struct FAT* fat, uint32_t cluster){
	return fat->firstDataSector + ((cluster - 2) * fat->headers.boot.secPerClus);
}

static uint32_t _get_cluster_entry(struct FAT32DirectoryEntry* entry){
	return (entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
}

static uint32_t _next_cluster(struct FAT* fat, uint32_t current){
	return fat->table[current] & 0x0FFFFFFF; // Mask to get the cluster number
}

static uint32_t _find_free_cluster(struct FAT* fat){
	if(!fat || !fat->table){
		return 0xFFFFFFFF;
	}

	uint32_t cluster;
	if(fat->fsInfo.nextFreeCluster >= fat->rootClus && fat->fsInfo.nextFreeCluster < fat->totalClusters){
		cluster = fat->fsInfo.nextFreeCluster;
		for (; cluster < fat->totalClusters; cluster++)
		{
			if(_next_cluster(fat, cluster) == FAT_UNUSED){
				return cluster;
			}
		}
	}

	cluster = fat->rootClus;
	for (; cluster < fat->totalClusters; cluster++)
	{
		if(_next_cluster(fat, cluster) == FAT_UNUSED){
			return cluster;
		}
	}

	return 0xFFFFFFFF;
}

static uint32_t _reserve_next_cluster(struct FAT* fat){
	uint32_t cluster = _find_free_cluster(fat);
	if(cluster == 0xFFFFFFFF){
		return cluster;
	}

	fat->fsInfo.nextFreeCluster = cluster + 1;
	if (fat->fsInfo.freeClusterCount != 0xFFFFFFFF) {
		fat->fsInfo.freeClusterCount--;
	}

	return cluster;
}

static int8_t _update(struct FAT* fat){
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

	if(_fsinfo_sig_valid(&fat->fsInfo)){
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

static int64_t _entry_lba(struct FAT* fat, struct FAT32DirectoryEntry* entry, uint32_t dirCluster){
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
		uint32_t lba = _cluster_to_lba(fat, cluster);
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

		cluster = _next_cluster(fat, cluster);
	}

out:
	stream_dispose(stream);
	return status;
}

static int8_t _save_entry(struct FAT* fat, struct FAT32DirectoryEntry* entry, uint32_t dirCluster){
	if(!entry || dirCluster < 2){
		return INVALID_ARG;
	}

	struct Stream* stream = stream_new();
	if(!stream){
		return NO_MEMORY;
	}

	int8_t status = SUCCESS;
	int lba = _entry_lba(fat, entry, dirCluster);
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

static int8_t _free_chain(struct FAT* fat, uint32_t start){
	if(!fat || start < 2 || start > fat->totalClusters){
		return INVALID_ARG;
	}

	int count = 0;
	uint32_t cur = start;
	while(1){
		if(!((cur <= 0) < fat->totalClusters)){
			break;
		}

		uint32_t next = _next_cluster(fat, cur);
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

	return _update(fat);
}

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
		uint32_t lba = _cluster_to_lba(fat, cluster);
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

		cluster = _next_cluster(fat, cluster);
	}

out:
	stream_dispose(stream);
	return status;
}

static int8_t _traverse_path(struct FAT *fat, const char *path, struct FAT32DirectoryEntry *buff){
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

		if((status = _find_entry(fat, token, _get_cluster_entry(buff), buff)) != SUCCESS){
			return status;
		}
	}

	return status;
}

static int8_t _create_entry(struct FAT *fat, const char *pathname, int attr){
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

	if(_traverse_path(fat, pathname, &buffer) == SUCCESS){
		status = FILE_EXISTS;
		goto out;
	}

	if(strlen(pathCopy) > 0){
		if ((status =  _traverse_path(fat, pathCopy, &buffer)) != SUCCESS){
			goto out;
		}

		if (buffer.DIR_Attr & ATTR_ARCHIVE){
			status = INVALID_ARG;
			goto out;
		}

		dirCluster = _get_cluster_entry(&buffer);
	}

	uint32_t cluster = dirCluster;
	while(1) {
		uint32_t lba = _cluster_to_lba(fat, cluster);
		stream_seek(stream, _SEC(lba), SEEK_SET);

		for(uint32_t i = 0; i < (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
			if(stream_read(stream, &buffer, sizeof(buffer)) != SUCCESS) {
				status = ERROR_IO;
				goto out;
			}

			if(buffer.DIR_Name[0] != 0x0 && buffer.DIR_Name[0] != 0xE5){
				continue;
			}

			uint32_t _cluster = _reserve_next_cluster(fat);
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

			status = _update(fat);
			goto out;
		}

		cluster = _next_cluster(fat, cluster);

		// Reserve a new cluster for the directory if necessary.
		if(CHK_EOF((cluster))){
			uint32_t new = _reserve_next_cluster(fat);
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

static int8_t _remove_entry(struct FAT *fat, const char *pathname){
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
		if ((status =  _traverse_path(fat, dirpath, &buffer)) != SUCCESS){
			return status;
		}

		if (buffer.DIR_Attr & ATTR_ARCHIVE){
			return INVALID_ARG;
		}

		dirCluster = _get_cluster_entry(&buffer);
	}

	if ((status = _traverse_path(fat, pathname, &buffer)) != SUCCESS)
	{
		stream_dispose(stream);
		return status;
	}

	_free_chain(fat, _get_cluster_entry(&buffer));
	int lba = _entry_lba(fat, &buffer, dirCluster);
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

	fat->fsInfo.nextFreeCluster = _get_cluster_entry(&buffer);

	stream_dispose(stream);
	return _update(fat);
}

static int8_t _truncate_entry(struct FAT* fat, struct FAT32DirectoryEntry* entry){
	if(!fat || !entry){
		return INVALID_ARG;
	}

	int8_t status = SUCCESS;
	uint32_t s = _get_cluster_entry(entry);
	uint32_t n = _next_cluster(fat, s);

	entry->DIR_FileSize = 0;
	if((status = _free_chain(fat, s)) != SUCCESS){
		return status;
	}

	if(_fsinfo_sig_valid(&fat->fsInfo)){
		if(!CHK_EOF(n)){
			fat->fsInfo.nextFreeCluster = n;
		}

		if(fat->fsInfo.freeClusterCount != 0xFFFFFFFF){
			fat->fsInfo.freeClusterCount--;
		}
	}

	return _update(fat);
}

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

    if (!_fsinfo_sig_valid(&fat->fsInfo)){
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
		if ((status =  _traverse_path(fat, dirname, &buffer)) != SUCCESS){
			return status;
		}

		if (buffer.DIR_Attr & ATTR_ARCHIVE){
			return INVALID_ARG;
		}

		dirCluster = _get_cluster_entry(&buffer);
	}

	status = _traverse_path(fat, pathname, &buffer);
	if(status == FILE_NOT_FOUND && (flags & O_CREAT)){
		if((status = _create_entry(fat, pathname, ATTR_ARCHIVE)) != SUCCESS){
			return status;
		}

		status = _traverse_path(fat, pathname, &buffer);
	}

	if(status != SUCCESS){
		return status;
	}

	if(flags & O_TRUNC){
		if(flags & O_WRONLY){
			if((status = _truncate_entry(fat, &buffer)) != SUCCESS){
				return status;
			}
		}
	}

	struct FATFileDescriptor* ffd = (struct FATFileDescriptor*)kcalloc(sizeof(struct FATFileDescriptor));
	if(!ffd){
		return NO_MEMORY;
	}

	ffd->firstCluster = _get_cluster_entry(&buffer);
	ffd->currentCluster = ffd->firstCluster;
	ffd->dirCluster = dirCluster;
	ffd->entry = buffer;
	*outFDPtr = (void*)ffd;

	return SUCCESS;
}

int FAT32_stat(struct FAT* fat, const char* restrict pathname, struct Stat* restrict statbuf){
	if(!fat || !statbuf || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	int status = SUCCESS;
	struct FAT32DirectoryEntry entry;
	if((status = _traverse_path(fat, pathname, &entry)) != SUCCESS){
		return status;
	}

	statbuf->fileSize = entry.DIR_FileSize;
	statbuf->attr = entry.DIR_Attr;
	statbuf->creDate = entry.DIR_CrtDate;
	statbuf->modDate = entry.DIR_WrtTime; 

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
    uint32_t lba = _cluster_to_lba(fat, ffd->currentCluster);

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
            uint32_t next = _next_cluster(fat, ffd->currentCluster);
            if(CHK_EOF(next)){
                status = END_OF_FILE; // Reached end of file
				goto out;
            }

            ffd->currentCluster = next;
            clusterOffset = cursor % CLUSTER_SIZE;
            bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;

            lba = _cluster_to_lba(fat, next);
            stream_seek(stream, _SEC(lba) + clusterOffset, SEEK_SET);
        }
    }

out:
	stream_dispose(stream);
    return totalReaded;
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
		cluster= _next_cluster(fat, cluster);
		if(CHK_EOF(cluster)) {
			return END_OF_FILE; // Reached end of file
		}
	}

	ffd->currentCluster = cluster;
	ffd->cursor = target;

	return SUCCESS;
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
    uint32_t lba = _cluster_to_lba(fat, ffd->currentCluster);

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
            uint32_t next = _next_cluster(fat, ffd->currentCluster);
            if(CHK_EOF(next)){
                next = _reserve_next_cluster(fat);
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

            lba = _cluster_to_lba(fat, next);
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
	if((status = _save_entry(fat, &ffd->entry, ffd->dirCluster)) != SUCCESS){
		return status;
	}

	if((status = _update(fat)) != SUCCESS){
		return status;
	}

    return totalWritten;
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

	return _create_entry(fat, pathname, ATTR_DIRECTORY);
}

int FAT32_rmdir(struct FAT* fat, const char* pathname){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	int8_t status;
	struct FAT32DirectoryEntry entry;
	if((status = _traverse_path(fat, pathname, &entry)) != SUCCESS){
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
	uint32_t cluster = _get_cluster_entry(&entry);
	int8_t result = SUCCESS;

	while (!CHK_EOF(cluster)) {
		uint32_t lba = _cluster_to_lba(fat, cluster);
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

		cluster = _next_cluster(fat, cluster);
	}

check_empty:
	if (entryCount > 2) {
		result = DIR_NOT_EMPTY;
	} else {
		result = _remove_entry(fat, pathname);
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

	return _create_entry(fat, pathname, ATTR_ARCHIVE);
}

int FAT32_remove(struct FAT* fat, const char* pathname){
	if(!fat || !pathname || strlen(pathname) > PATH_MAX){
		return INVALID_ARG;
	}

	int8_t status;
	struct FAT32DirectoryEntry entry;
	if((status = _traverse_path(fat, pathname, &entry)) != SUCCESS){
		return status;
	}

	if(entry.DIR_Attr & ATTR_DIRECTORY){
		return NOT_SUPPORTED; // Cannot remove a directory
	}

	return _remove_entry(fat, pathname); // Remove a file or directory
}
