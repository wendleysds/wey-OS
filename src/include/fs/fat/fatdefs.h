#ifndef _FAT_DEFS_H
#define _FAT_DEFS_H

#include <stdint.h>

#define FAT_SIGNATURE 0x29
#define FAT_ENTRY_SIZE 0x02
#define FAT_BAD_SECTOR 0xFF7
#define FAT_UNUSED 0x00

// FAT File/Directory attributes
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

#define ATTR_LONG_NAME \
	ATTR_READ_ONLY | ATTR_HIDDEN \
	| ATTR_SYSTEM |ATTR_VOLUME_ID

#define ATTR_LONG_NAME_MASK \
	ATTR_READ_ONLY | ATTR_HIDDEN \
	|ATTR_SYSTEM |ATTR_VOLUME_ID \
	|ATTR_DIRECTORY |ATTR_ARCHIVE

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
}__attribute__((packed));

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
}__attribute__((packed));

struct FAT32DirectoryEntry{
	uint8_t DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t NTRes;
	uint8_t DIR_CrtTimeTenth;
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	uint16_t DIR_FstClusHI;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint16_t DIR_FstClusLO;
	uint32_t DIR_FileSize;
}__attribute__((packed));

struct FAT32FSInfo {
    uint32_t leadSignature; // 0x41615252 ("RRaA")
    uint8_t reserved1[480];
    uint32_t structSignature; // 0x61417272 ("rrAa")
    uint32_t freeClusterCount;
    uint32_t nextFreeCluster;
    uint8_t reserved2[12];
    uint32_t trailSignature; // 0xAA550000 (or AA55h in the end)
} __attribute__((packed));

struct FATLongDirectoryEntry{
	uint8_t LDIR_Ord;
	uint16_t LDIR_Name1[5]; // Characters 1-5
	uint8_t LDIR_Attr; // Must be ATTR_LONG_NAME
	uint8_t LDIR_Type;
	uint8_t LDIR_Chksum;
	uint16_t LDIR_Name2[6]; // Characters 6-11
	uint16_t LDIR_FstClusLO; // Must be 0
	uint16_t LDIR_Name3[2]; // Characters 12-13
}__attribute__((packed));

struct FATHeaders{
	struct FATHeader boot;
	struct FAT32HeaderExtended extended;
}__attribute__((packed));

enum ItemType{
	File,
	Directory,
};

struct Directory{
	struct FAT32DirectoryEntry* entry;
	uint16_t itensCount;
	uint32_t firstCluster;
	uint32_t currentCluster;
}__attribute__((packed));

struct FATItem{
	enum ItemType type;
	int offsetInBytes; // LBA of the item for updating
	union{
		struct FAT32DirectoryEntry* file;
		struct Directory* directory;
	};
}__attribute__((packed));

struct FATFileDescriptor{
	struct FATItem* item;
	uint32_t cursor;
	uint32_t firstCluster;
	uint32_t currentCluster;
}__attribute__((packed));

struct FAT{
	struct FATHeaders headers;
	struct FAT32FSInfo fsInfo;
	struct Directory rootDir;

	uint32_t* table;
	uint32_t totalClusters;
	uint32_t firstDataSector;

	// Streams for reading and writing
	struct Stream* readStream;  // Gerenal Purpose Read Stream
	struct Stream* writeStream; // General Purpose Write Stream
	struct Stream* clusterReadStream;
}__attribute__((packed));

#endif
