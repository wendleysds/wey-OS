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

static uint32_t fat_get_file_cluster(fat_info_t* fat, const char* filename, uint32_t dirCluster){
	uint32_t clusterSize = fat->headers.boot.bytesPerSec * fat->headers.boot.secPerClus;

	char buffer[clusterSize];
	uint32_t currentCluster = dirCluster;
	
	do{
		uint32_t lba = cluster_to_lba(fat, currentCluster);
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

			if(memcmp(entry->name, filename, 11) == 0){
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
	return 0x0;
}

static int _write(struct file *file, const void *buffer, uint64_t count){
	return NOT_SUPPORTED;
}

static int _lseek(struct file *file, uint64_t offset, uint8_t whence){
	return 0x0;
}

static int _close(struct file *file){
	return 0x0;
}

static const struct file_operations ops = {
	.read = _read,
	.write = _write,
	.lseek = _lseek,
	.close = _close
};

struct file* platform_open_file(fat_info_t* fat_info, const char* path){
	struct file* file = NULL;
	
	if(!file){
		return ERR_PTR(NO_MEMORY);
	}

	file->ops = &ops;
	return file;
}
