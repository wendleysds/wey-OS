#ifndef _DISK_H
#define _DISK_H

#include <def/compile.h>
#include <stdint.h>

struct disk;

// mbr
struct MBRPartitionEntry{
	uint8_t status;
	uint8_t chsFirst[3];
	uint8_t type;
	uint8_t chsLast[3];
	uint32_t lbaFirstSector;
	uint32_t totalSectors;
} __packed;

// gpt
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
	uint8_t diskGUID[16];
	uint64_t partEntryStartingLBA;
	uint32_t numPartEntries;
	uint32_t partEntrySize;
	uint32_t partEntryArrayCRC32;
} __packed;

struct disk_op {
	int (*read)(struct disk* disk, uint32_t lba, void* buffer, uint8_t seccount);
	int (*write)(struct disk* disk, uint32_t lba, const void* buffer, uint8_t seccount);
} __packed;

struct disk {
	uint8_t id;

	void* private;

	struct GPTHeader* gpt_header;

	union {
		struct MBRPartitionEntry* mbr_partitions;
		struct GPTPartitionEntry* gpt_partitions;
	};
	
	struct disk_op* ops;
} __packed;

extern struct disk* main_disk;

void disk_register(struct disk* disk);
void disk_unregister(struct disk* disk);

#endif
