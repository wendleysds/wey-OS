#include <def/compile.h>
#include <def/err.h>
#include <utils.h>
#include <string.h>
#include <fat.h>

#include <stdint.h>

// Read-Only and utils fat32 driver

/*struct MBRPartitionEntry{
	uint8_t status;
	uint8_t chsFirst[3];
	uint8_t type;
	uint8_t chsLast[3];
	uint32_t lbaFirstSector;
	uint32_t totalSectors;
} __packed;

struct FATHeaders headers;

struct MBRPartitionEntry mbrEntries[4];

extern asmlinkage int disk_read(uint32_t lba, uint16_t seg, uint16_t off, uint16_t seccount);

int init_fat(){
	if(disk_read(0x0, SEG(&mbrEntries), OFF(&mbrEntries), 1) != 0){
		return ERROR_IO;
	}

	if(mbrEntries[0].type != 0x0B && mbrEntries[0].type != 0x0C){
		return INVALID_ARG;
	}

	return disk_read(mbrEntries[0].lbaFirstSector, SEG(&headers), OFF(&headers), 1) == 1 ? ERROR_IO : SUCCESS;
	return 0;
}

uint32_t cluster_to_lba(uint32_t cluster){
	uint32_t firstDataSector = headers.boot.rsvdSecCnt + (headers.boot.numFATs * headers.fat32.FATSz32);
	uint32_t lba = firstDataSector + ((cluster - 2) * headers.boot.secPerClus);
	return lba;
}

uint32_t next_cluster(uint32_t cluster){
	uint32_t fatOffset = cluster * 4;
	uint32_t fatSecNum = headers.boot.rsvdSecCnt + (fatOffset / headers.boot.bytesPerSec);
	uint32_t entOffset = fatOffset % headers.boot.bytesPerSec;

	uint8_t sector[512];
	if(disk_read(fatSecNum, SEG(sector), OFF(sector), 1) != 0){
		return EOF;
	}

	uint32_t* table = (uint32_t*)&sector[entOffset];
	return (*table) & 0x0FFFFFFF;
}

uint32_t find_file_lba(const char* filename, uint32_t dirCluster){
	uint32_t clusterSize = headers.boot.bytesPerSec * headers.boot.secPerClus;

	char buffer[clusterSize];
	uint32_t currentCluster = dirCluster;
	
	do{
		uint32_t lba = cluster_to_lba(currentCluster);
		if(disk_read(lba, SEG(buffer), OFF(buffer), 1) != 0){
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
				return cluster_to_lba((entry->fstClusHI << 16) | entry->fstClusLO);
			}
		}

		currentCluster = next_cluster(currentCluster);
	} while (!(currentCluster >= EOF));

	return EOF;
}
*/