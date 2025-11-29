#include <def/compile.h>
#include <def/err.h>
#include <utils.h>
#include <heap.h>
#include <disk.h>
#include <string.h>
#include <platform/fat.h>
#include <file.h>

#include <stdint.h>

// Read-Only and utils fat32 driver

#define MAX_BASENAME 8
#define MAX_EXTENSION 3
#define DIR_ENTRY_SIZE 11
#define ILLEGAL_CHARS "\"*+,./:;<=>?[\\]|"
#define ILLEGAL_CHARS_REPLACE '_'

#define FILE_POOL_SIZE 4

struct fd{
	uint32_t startCluster;
	uint32_t currentCluster;
	fat_info_t* fat;
};

static struct fd* fd_pool = NULL;
static struct file* files_pool = NULL;
static uint8_t pool_used[FILE_POOL_SIZE];

static void file_pool_init(){
	if (files_pool) return;
	files_pool = (struct file*)malloc(sizeof(struct file) * FILE_POOL_SIZE);
	if (files_pool) {
		memset(files_pool, 0, sizeof(struct file) * FILE_POOL_SIZE);
	}

	fd_pool = (struct fd*)malloc(sizeof(struct fd) * FILE_POOL_SIZE);
	if (fd_pool) {
		memset(fd_pool, 0, sizeof(struct fd) * FILE_POOL_SIZE);
	}

	memset(pool_used, 0, sizeof(pool_used));
}

static struct file* file_get(){
	if (!files_pool) file_pool_init();
	if (!files_pool) return NULL;

	for (uint8_t i = 0; i < FILE_POOL_SIZE; ++i){
		if (!pool_used[i]){
			pool_used[i] = 1;
			struct file* f = &files_pool[i];
			memset(f, 0, sizeof(*f));
			f->private = &fd_pool[i];
			memset(&fd_pool[i], 0, sizeof(struct fd));
			return f;
		}
	}

	return NULL;
}

static void file_ret(struct file* f){
	if (!files_pool || !f) return;
	ptrdiff_t idx = f - files_pool;
	if (idx < 0 || (uint32_t)idx >= FILE_POOL_SIZE) return;
	pool_used[idx] = 0;
	memset(&files_pool[idx], 0, sizeof(struct file));
}

static int8_t _valid_fat_sector(const uint8_t* sector0){
	if (sector0[0] != 0xEB && sector0[0] != 0xE9) {
		return 0;
	}

	if (sector0[510] != 0x55 || sector0[511] != 0xAA){
		return 0;
	}

	int valid_oem = 1;
	for (int i = 3; i < 11; ++i) {
		if (sector0[i] < 0x20 || sector0[i] > 0x7E) {
			valid_oem = 0;
			break;
		}
	}

	return valid_oem;
}

static inline int _toupper(int c){
	if(c >= 'a' && c <= 'z'){
		c -= 32;
	}

	return c;
}

static inline char _convert_char(char c){
	if(c < 0x20 || strchr(ILLEGAL_CHARS, c)){
		return ILLEGAL_CHARS_REPLACE;
	}

	return _toupper((unsigned char)c);
}

static inline void _rtrim(char *str){
	int len = strlen(str);
	while(len > 0 && str[len - 1] == ' '){
		str[--len] = '\0';
	}
}

static void fat_name_generate_short(const char *name, char *out){
	char base[MAX_BASENAME + 1] = {0};
	char ext[MAX_EXTENSION + 1] = {0};
	const char *dot = strrchr(name, '.');

	if(dot && dot != name){
		for(int i = 0; i < MAX_EXTENSION && dot[1 + i] != '\0'; i++){
			ext[i] = _convert_char(dot[1 + i]);
		}
	}

	int i = 0, j = 0;
	for(i = 0; name[i] != '\0' && name + i != dot && j < MAX_BASENAME; i++){
		base[j++] = _convert_char(name[i]);
	}

	base[j] = '\0';
	_rtrim(base);

	memset(out, ' ', DIR_ENTRY_SIZE);
	memcpy(out, base, strlen(base));
	memcpy(out + 8, ext, strlen(ext));
}

static uint32_t next_cluster(fat_info_t* fat, uint32_t cluster){
	uint32_t fatOffset = cluster * 4;
	uint32_t fatSecNum = 
		fat->headers.boot.rsvdSecCnt + (fatOffset / fat->headers.boot.bytesPerSec);
	uint32_t entOffset = fatOffset % fat->headers.boot.bytesPerSec;

	uint8_t sector[512];
	if(IS_STAT_ERR(fat->disk->ops->read(fat->disk, fatSecNum, sector, 1))){
		return EOF;
	}

	uint32_t* table = (uint32_t*)&sector[entOffset];
	return (*table) & 0x0FFFFFFF;
}

static uint32_t cluster_to_lba(fat_info_t* fat, uint32_t cluster){
	uint32_t firstDataSector = 
		fat->headers.boot.rsvdSecCnt + 
		(fat->headers.boot.numFATs * fat->headers.fat32.FATSz32);

	uint32_t lba = firstDataSector + ((cluster - 2) * fat->headers.boot.secPerClus);
	return lba;
}

static uint32_t fat_get_file_cluster(fat_info_t* fat, const char* filename, uint32_t dirCluster, uint32_t* fsize){
	uint32_t clusterSize = fat->headers.boot.bytesPerSec * fat->headers.boot.secPerClus;

	char buffer[512];
	uint32_t currentCluster = dirCluster;

	char formated_name[12];
    memset(formated_name, 0x0, sizeof(formated_name));
    fat_name_generate_short(filename, formated_name);
	
	do{
		uint32_t lba = cluster_to_lba(fat, currentCluster) + fat->disk->mbr_partitions[0].lbaFirstSector;
		if(IS_STAT_ERR(fat->disk->ops->read(fat->disk, lba, buffer, 1))){
			return EOF;
		}

		for(int i = 0; i < (clusterSize / sizeof(struct FATLegacyEntry)); i++){
			struct FATLegacyEntry* entry = (struct FATLegacyEntry*)&buffer[i * sizeof(struct FATLegacyEntry)];

			if(entry->name[0] == 0x00){
				return EOF;	
			}

			if(entry->name[0] == 0xE5){
				continue;
			}

			if((entry->attr & 0x0F) == 0x0F){
				continue;
			}

			if(memcmp(entry->name, formated_name, 11) == 0){
				if(fsize) *fsize = entry->fileSize;
				return ((entry->fstClusHI << 16) | entry->fstClusLO);
			}
		}

		currentCluster = next_cluster(fat, currentCluster);
	} while (!(currentCluster >= EOF));

	return EOF;
}

fat_info_t* platform_parse_fat(struct disk* disk){
	uint8_t sec0[0x200];
	if(disk->ops->read(disk, disk->mbr_partitions[0].lbaFirstSector, sec0, 1) != SUCCESS){
		return ERR_PTR(READ_FAIL);
	}

	if(!_valid_fat_sector(sec0)){
        return ERR_PTR(INVALID_FS);
    }

	fat_info_t* fat = (fat_info_t*)malloc(sizeof(fat_info_t));
	if(!fat){
		return ERR_PTR(NO_MEMORY);
	}

	memcpy(&fat->headers, sec0, sizeof(struct FATHeaders));
	fat->disk = disk;

	return fat;
}

static int _read(struct file *file, void *buffer, uint64_t count){
	if(!file || !buffer || count == 0){
		return INVALID_ARG;
	}

	struct fd* fd = (struct fd*)file->private;
	fat_info_t* fat = fd->fat;

	uint32_t totalReaded = 0;

	if(!fat) return INVALID_ARG;

	if (file->pos >= file->size) {
		return END_OF_FILE;
	}

	uint32_t cursor = file->pos;
	uint32_t remaining = (uint32_t)count;

	/* clamp remaining to file size */
	if (remaining > file->size - cursor) {
		remaining = file->size - cursor;
		if (remaining == 0) return END_OF_FILE;
	}

	uint32_t clusterSize = fat->headers.boot.bytesPerSec * fat->headers.boot.secPerClus;
	uint32_t cluster = fd->currentCluster;

	/* offset inside the first cluster to read from */
	uint32_t offInCluster = cursor % clusterSize;
	uint32_t bytesLeftInCluster = clusterSize - offInCluster;

	uint8_t clusterBuf[clusterSize];
	memset(clusterBuf, 0x0, clusterSize);

	while (remaining > 0) {
		uint32_t lba = cluster_to_lba(fat, cluster);
		if (IS_STAT_ERR(fat->disk->ops->read(fat->disk, lba, clusterBuf, fat->headers.boot.secPerClus))) {
			return READ_FAIL;
		}

		uint32_t toCopy = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;
		memcpy((uint8_t*)buffer + totalReaded, clusterBuf + offInCluster, toCopy);

		totalReaded += toCopy;
		remaining -= toCopy;
		cursor += toCopy;

		/* after first iteration, subsequent reads start at cluster offset 0 */
		offInCluster = 0;
		bytesLeftInCluster = clusterSize;

		if (remaining > 0) {
			cluster = next_cluster(fat, cluster);
			if (cluster >= EOF) {
				/* reached end of cluster chain prematurely */
				file->pos = file->size;
				return totalReaded;
			}
		}
	}

	/* update current cluster and file position */
	fd->currentCluster = cluster;
	file->pos = cursor;

	return totalReaded;
}


static int _lseek(struct file *file, uint64_t offset, uint8_t whence){
	if(!file){
		return INVALID_ARG;
	}

	struct fd* fd = (struct fd*)file->private;
	uint32_t filesize = file->size;

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

	uint32_t clusterSize = fd->fat->headers.boot.bytesPerSec * fd->fat->headers.boot.secPerClus;
	uint32_t clusterOffset = target / clusterSize;

	uint32_t cluster = fd->startCluster;
	for(uint32_t i = 0; i < clusterOffset; i++){
		cluster = next_cluster(fd->fat, cluster);
		if(cluster >= EOF) {
			return END_OF_FILE; // Reached end of file
		}
	}

	fd->currentCluster = cluster;
	file->pos = target;

	return filesize - target;
}

static int _close(struct file *file){
	file_ret(file);
	return 0;
}

static const struct file_operations ops = {
	.read = _read,
	.lseek = _lseek,
	.close = _close
};

struct file* platform_open_file(fat_info_t* fat_info, const char* path){
	struct file* file = file_get();
	
	if(!file){
		return ERR_PTR(NOT_FOUND); // pull empty
	}
	
	char buffer[128];
	strcpy(buffer, path);

	char* token = strtok(buffer, "/");

	uint32_t dirCluster = fat_info->headers.fat32.rootClus;

	do{
		dirCluster = fat_get_file_cluster(fat_info, token, dirCluster, &file->size);
		if(dirCluster >= EOF){
			file_ret(file);
			return ERR_PTR(FILE_NOT_FOUND);
		}
	} while ((token = strtok(NULL, "/")));

	struct fd* fd = (struct fd*)file->private;

	fd->startCluster = fd->currentCluster = dirCluster;
	fd->fat = fat_info;

	file->ops = &ops;
	return file;
}
