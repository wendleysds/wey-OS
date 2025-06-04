#ifndef _ATA_LBA_H
#define _ATA_LBA_H

#include <stdint.h>

int ata_read_sectors(uint32_t lba, uint8_t totalSectors, void* buffer);

#endif

