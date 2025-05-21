#include <fs/fat/fat32.h>
#include <stdint.h>

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
	uint16_t fATSz16;
	uint16_t secPerTrk;
	uint16_t numHeads;
	uint32_t hiddSec;
	uint32_t toSec32;
}__attribute__((packed));

struct FATHeaderExtended{
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
}__attribute__((packed));

