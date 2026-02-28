#ifndef _PARTITON_H
#define _PARTITON_H

#include <def/compile.h>
#include <stdint.h>

#define MBR_PARTITION_TABLE     0x1BE
#define MBR_PARTITION_COUNT     4
#define MBR_PARTITION_ENTRY_SZ  16
#define MBR_SIGNATURE_OFFSET    0x1FE
#define MBR_SIGNATURE_VALUE     0xAA55

#define MBR_TYPE_EMPTY        0x00
#define MBR_TYPE_FAT12        0x01
#define MBR_TYPE_FAT16        0x06
#define MBR_TYPE_EXTENDED     0x05
#define MBR_TYPE_LINUX        0x83
#define MBR_TYPE_LINUX_SWAP   0x82
#define MBR_TYPE_GPT_PROTECT  0xEE

#define MBR_PARTITION_ENTRY(mbr, index) \
    ((uint8_t*)(mbr) + MBR_PARTITION_TABLE + \
    ((index) * MBR_PARTITION_ENTRY_SZ))

#define GPT_HEADER_SIGNATURE      "EFI PART"
#define GPT_HEADER_LBA            1
#define GPT_HEADER_SIGNATURE_SZ   8

#define GPT_SIGNATURE_OFFSET      0x00
#define GPT_REVISION_OFFSET       0x08
#define GPT_HEADER_SIZE_OFFSET    0x0C
#define GPT_CURRENT_LBA_OFFSET    0x18
#define GPT_BACKUP_LBA_OFFSET     0x20
#define GPT_FIRST_USABLE_OFFSET   0x28
#define GPT_LAST_USABLE_OFFSET    0x30
#define GPT_DISK_GUID_OFFSET      0x38
#define GPT_PART_LBA_OFFSET       0x48
#define GPT_PART_COUNT_OFFSET     0x50
#define GPT_PART_SIZE_OFFSET      0x54
#define GPT_PART_ARRAY_CRC_OFFSET 0x58

#define GPT_U32(ptr, off) (*(uint32_t*)((uint8_t*)(ptr) + (off)))
#define GPT_U64(ptr, off) (*(uint64_t*)((uint8_t*)(ptr) + (off)))

#define GPT_PART_ENTRY(array, size, index) \
    ((uint8_t*)(array) + ((index) * (size)))

struct MBRPartitionEntry{
	uint8_t status;
	uint8_t chsFirst[3];
	uint8_t type;
	uint8_t chsLast[3];
	uint32_t lbaStart;
	uint32_t totalSectors;
} __packed;

struct GPTPartitionEntry{
	uint8_t typeGUID[16];
	uint8_t uniqueGUID[16];
	uint64_t firstLBA;
	uint64_t lastLBA;
	uint64_t attributes;
	uint16_t name[36];
} __packed;

struct GPTHeader{
	uint64_t signature;
	uint32_t revision;
	uint32_t headerSize;
	uint32_t headerCRC32;
	uint32_t reserved;
	uint64_t currentLBA;
	uint64_t backupLBA;
	uint64_t firstUsableLBA;
	uint64_t lastUsableLBA;
	uint8_t  diskGUID[16];
	uint64_t partEntryStartingLBA;
	uint32_t numPartEntries;
	uint32_t partEntrySize;
	uint32_t partEntryArrayCRC32;
} __packed;

typedef enum {
	PARTITION_MBR,
	PARTITION_GPT,
} partition_entry_t;

int partition_detect(void *sector0, partition_entry_t *out_type);
int mbr_is_valid(void *sector0);
int gpt_is_valid(void *sector1);

#endif
