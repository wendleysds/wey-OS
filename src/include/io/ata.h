#ifndef _ATA_LBA_H
#define _ATA_LBA_H

#include <stdint.h>

struct ATAIdentifyDevice {
	uint16_t generalConfig;             // 0
	uint16_t obsolete1;                  // 1
	uint16_t specificConfig;            // 2
	uint16_t obsolete2[7];               // 3-9
	char     serialNumber[20];          // 10-19
	uint16_t obsolete3[3];               // 20-22
	char     firmwareRevision[8];       // 23-26
	char     modelNumber[40];           // 27-46
	uint16_t maxSectorsPerInterrupt;  // 47
	uint16_t capabilities[2];            // 48-49
	uint16_t obsolete4[2];               // 50-51
	uint16_t fieldValidity;             // 53
	uint16_t currentCylinders;          // 54
	uint16_t currentHeads;              // 55
	uint16_t currentSectorsPerTrack;  // 56
	uint16_t currentSectorCapacity[2]; // 57-58
	uint16_t multiSectorSettings;      // 59
	uint32_t totalLBA28Sectors;        // 60-61
	uint16_t obsolete5[38];              // 62–99
	uint64_t totalLBA48Sectors;        // 100–103
	uint16_t reserved[22];               // 104–125
	uint16_t commandSetsSupported[3];  // 82–84
	uint16_t commandSetsEnabled[3];    // 85–87
	uint16_t ultraDMAModes;            // 88
	uint16_t moreReserved[128 - 89];    // 89–127
	uint16_t reservedVendor[128];       // 128–255
} __attribute__((packed));

int ata_read_sectors(uint32_t lba, uint8_t totalSectors, void* buffer);
int ata_read_sectors_28(uint32_t lba, uint8_t totalSectors, void* buffer);
int ata_read_sectors_48(uint64_t lba, uint16_t totalSectors, void* buffer);

#endif

