#ifndef _FAT_H
#define _FAT_H

#include <def/compile.h>
#include <disk.h>
#include <stdint.h>

#define EOF 0xFFFFFFF8

struct FATHeader{
	uint8_t jmpBoot[3];
	uint8_t OEMName[8];
	uint16_t bytesPerSec;
	uint8_t secPerClus;
	uint16_t rsvdSecCnt;
	uint8_t numFATs;
	uint16_t rootEntCnt;
	uint16_t totSec16;
	uint8_t mediaType;
	uint16_t FATSz16;
	uint16_t secPerTrk;
	uint16_t numHeads;
	uint32_t hiddSec;
	uint32_t totSec32;
} __packed;

struct FAT32HeaderExtended{
	uint32_t FATSz32;
	uint16_t extFlags;
	uint16_t FSVer;
	uint32_t rootClus;
	uint16_t FSInfo;
	uint16_t BkBootSec;
	uint8_t reserved[12];
	uint8_t drvNum;
	uint8_t reserved1;
	uint8_t bootSig;
	uint32_t volID;
	uint8_t volLab[11];
	uint8_t filSysType[8];
} __packed;

struct FATLegacyEntry{
	uint8_t name[11];
	uint8_t attr;
	uint8_t NTRes;
	uint8_t crtTimeTenth;
	uint16_t crtTime;
	uint16_t crtDate;
	uint16_t lstAccDate;
	uint16_t fstClusHI;
	uint16_t wrtTime;
	uint16_t wrtDate;
	uint16_t fstClusLO;
	uint32_t fileSize;
} __packed;

struct FATHeaders{
	struct FATHeader boot;
	struct FAT32HeaderExtended fat32;
};

typedef struct {
	struct FATHeaders headers;
	struct disk* disk;
} fat_info_t;

#endif
