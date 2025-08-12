#ifndef _FAT_INTERNALS_H
#define _FAT_INTERNALS_H

#include <io/stream.h>
#include <fs/vfs.h>
#include <stdint.h>

#define _SEC(lba) \
    (lba * SECTOR_SIZE)

#define FAT_INVAL 0xFFFFFFFF

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

typedef enum {
	FAT_TYPE_12,
    FAT_TYPE_16,
    FAT_TYPE_32,
	FAT_INVALID,
	FAT_TYPES_COUNT
} fat_type_t;

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

struct FATHeaderExtended{
	uint8_t drvNum;
    uint8_t reserved1;
    uint8_t bootSig;
    uint32_t volID;
    uint8_t volLab[11];
    uint8_t filSysType[8];
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

struct FATLegacyEntry{
	uint8_t name[11];
	uint8_t attr;
	uint8_t NTRes;
	uint8_t CrtTimeTenth;
	uint16_t CrtTime;
	uint16_t CrtDate;
	uint16_t LstAccDate;
	uint16_t FstClusHI;
	uint16_t WrtTime;
	uint16_t WrtDate;
	uint16_t FstClusLO;
	uint32_t fileSize;
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

struct FATLongEntry{
	uint8_t Ord;
	uint16_t Name1[5]; // Characters 1-5
	uint8_t Attr; // Must be ATTR_LONG_NAME
	uint8_t Type;
	uint8_t Chksum;
	uint16_t Name2[6]; // Characters 6-11
	uint16_t FstClusLO; // Must be 0
	uint16_t Name3[2]; // Characters 12-13
}__attribute__((packed));

struct FATHeaders{
	struct FATHeader boot;
	union {
		struct FATHeaderExtended fat12;
		struct FATHeaderExtended fat16;
		struct FAT32HeaderExtended fat32;
	} extended;
};

struct FAT{
	fat_type_t type;
	struct FATHeaders headers;

	union {
		uint8_t*  fat12;
		uint16_t* fat16;
		uint32_t* fat32;
	} table;

	struct FAT32FSInfo fsInfo;

	union 
	{
		uint32_t rootClus; 
		uint16_t rootSector;
	};

	uint32_t clusterSize;
	uint32_t totalClusters;
	uint32_t firstDataSector;

	struct Stream* stream;
};

struct FATFileDescriptor{
	struct FATLegacyEntry entry;
	struct FAT* fat;
	uint32_t firstCluster;
	uint32_t currentCluster;
	uint32_t parentDirCluster;
};

typedef int (*_fat_loader_func_t)(struct FAT*, struct Stream* stream, const uint8_t* sector0Buffer);

extern _fat_loader_func_t loaders[];
extern struct inode_operations vfat_fs_iop;
extern struct file_operations vfat_fs_fop;

// vfat dir
struct inode* fat_lookup(struct inode *dir, const char *name);
int fat_mkdir(struct inode *dir, const char *name);
int fat_rmdir(struct inode *dir, const char *name);

// vfat io
int fat_read(struct file *file, void *buffer, uint32_t count);
int fat_write(struct file *file, const void *buffer, uint32_t count);
int fat_lseek(struct file *file, int offset, int whence);
int fat_close(struct file *file);

int fat_update(struct FAT* fat);

// vfat files
int fat_create(struct inode *dir, const char *name, uint16_t mode);
int fat_unlink(struct inode *dir, const char *name);
int fat_getattr(struct inode *dir, const char *name, struct stat* restrict statbuf);
int fat_setarrt(struct inode *dir, const char *name, uint16_t attr);

int _fat_create(struct FAT* fat, const char* filename, uint32_t dirCluster, int attr);
int _fat_remove(struct FAT* fat, const char* filename, uint32_t dirCluster);

// Name gen
void fat_name_append_tilde(char *outname, int n);
void fat_name_generate_short(const char *name, char *out);

// Utils
uint8_t fat32_fsinfo_sig_valid(struct FAT32FSInfo* fsInfo);
uint32_t fat_next_cluster(struct FAT* fat, uint32_t cluster);
uint32_t fat_find_free_cluster(struct FAT* fat);

void fat_add(struct FAT* fat, uint32_t index, uint32_t next);
uint8_t fat_is_eof(struct FAT* fat, uint32_t cluster);
uint32_t fat_get_eof(struct FAT* fat);

uint32_t fat_append_cluster(struct FAT* fat, uint32_t from);
void fat_free_chain(struct FAT* fat, uint32_t startCluster);

int64_t fat_get_entry_lba(struct FAT* fat, uint32_t dirCluster, struct FATLegacyEntry* entry);

static inline uint32_t fat_cluster_to_lba(struct FAT* fat, uint32_t cluster){
	return fat->firstDataSector + ((cluster - 2) * fat->headers.boot.secPerClus);
}

#endif
