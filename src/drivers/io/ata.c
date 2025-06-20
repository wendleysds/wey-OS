#include <io/ata.h>
#include <io/ports.h>
#include <def/status.h>
#include <stdint.h>

#define TRIES 100000
#define WORDS_PER_SECTOR 256

// ATA Primary ports
#define DATA_RESGISTE 0x1F0
#define ERROR_REGISTER 0x1F1
#define FEATURES_REGISTER 0x1F1
#define SECTOR_COUNT_REGISTER 0x1F2
#define SECTOR_NUMBER_REGISTER 0x1F3
#define	CYLINDER_LOW_REGISTER 0x1F4
#define CYLINDER_HIGH_REGISTER 0x1F5
#define DRIVE_HEAD_REGISTER 0x1F6
#define STATUS_REGISTER 0x1F7
#define COMMAND_REGISTER 0x1F7

// ATA Stats
#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DRQ     0x08
#define ATA_SR_ERR     0x01

/*
 * Reads sectors from an ATA disk using LBA addressing.
 *
 * Reading is done through direct communication with the 
 * standard I/O ports of the ATA interface (primary channel, master).
 */
int ata_read_sectors(uint32_t lba, uint8_t totalSectors, void* buffer){
	return ata_read_sectors_28(lba, totalSectors, buffer);
}

/*
 * Write sectors from an ATA disk using LBA addressing.
 *
 * Writing is done through direct communication with the 
 * standard I/O ports of the ATA interface (primary channel, master).
 */
int ata_write_sectors(uint32_t lba, uint8_t totalSectors, void* buffer){
	return ata_write_sectors_28(lba, totalSectors, buffer);
}

int ata_read_sectors_28(uint32_t lba, uint8_t totalSectors, void* buffer){
	// Wait until disk be ready
	int t = TRIES;
	uint8_t status;
	do{
		status = inb_p(STATUS_REGISTER);
		if(!t--)
			return TIMEOUT;
	} while (status & ATA_SR_BSY);

	outb_p(DRIVE_HEAD_REGISTER, (lba >> 24) | 0xE0);       // Drive + LBA bits
	outb_p(SECTOR_COUNT_REGISTER, totalSectors);
	outb_p(SECTOR_NUMBER_REGISTER, (uint8_t)(lba & 0xFF)); // Low
	outb_p(CYLINDER_LOW_REGISTER, (uint8_t)(lba >> 8));    // Mid
	outb_p(CYLINDER_HIGH_REGISTER, (uint8_t)(lba >> 16));  // High
	outb_p(COMMAND_REGISTER, 0x20); // Read
	
	uint16_t* ptr = (uint16_t*)buffer;
	for(uint8_t sector = 0; sector < totalSectors; sector++){
		t = TRIES;
		do {
			status = inb_p(STATUS_REGISTER);

			if (status & ATA_SR_ERR)
				return ERROR;
			if (!t--)
				return TIMEOUT;
		} while (!(status & ATA_SR_DRQ));

		insw(DATA_RESGISTE, ptr, WORDS_PER_SECTOR);
		ptr += WORDS_PER_SECTOR;
	}

	return SUCCESS;
}

int ata_read_sectors_48(uint64_t lba, uint16_t totalSectors, void* buffer){
	// Wait until disk be ready
	int t = TRIES;
	uint8_t status;
	do{
		status = inb_p(STATUS_REGISTER);
		if(!t--)
			return TIMEOUT;
	} while (status & ATA_SR_BSY);

	outb_p(SECTOR_COUNT_REGISTER, (totalSectors >> 8) & 0xFF);
	outb_p(SECTOR_NUMBER_REGISTER, (lba >> 24) & 0xFF);
	outb_p(CYLINDER_LOW_REGISTER, (lba >> 32) & 0xFF);
	outb_p(CYLINDER_HIGH_REGISTER, (lba >> 40) & 0xFF);

	outb_p(SECTOR_COUNT_REGISTER, totalSectors & 0xFF);
	outb_p(SECTOR_NUMBER_REGISTER, (lba >> 0) & 0xFF);
	outb_p(CYLINDER_LOW_REGISTER, (lba >> 8) & 0xFF);
	outb_p(CYLINDER_HIGH_REGISTER, (lba >> 16) & 0xFF);

	outb_p(DRIVE_HEAD_REGISTER, 0x40);

	outb_p(COMMAND_REGISTER, 0x24);

	uint16_t* ptr = (uint16_t*)buffer;
	for(uint8_t sector = 0; sector < totalSectors; sector++){
		t = TRIES;
		do {
			status = inb_p(STATUS_REGISTER);

			if (status & ATA_SR_ERR)
				return ERROR;
			if (!t--)
				return TIMEOUT;
		} while (!(status & ATA_SR_DRQ));

		insw(DATA_RESGISTE, ptr, WORDS_PER_SECTOR);
		ptr += WORDS_PER_SECTOR;
	}

	return SUCCESS;
}

int ata_write_sectors_28(uint32_t lba, uint8_t totalSectors, void* buffer){
	if (totalSectors == 0)
    	return INVALID_ARG;

	int t = TRIES;
	uint8_t status;
	do{
		status = inb_p(STATUS_REGISTER);
		if(!t--)
			return TIMEOUT;
	} while (status & ATA_SR_BSY);

	outb_p(DRIVE_HEAD_REGISTER, (lba >> 24) | 0xE0);       // Drive + LBA bits
	outb_p(SECTOR_COUNT_REGISTER, totalSectors);
	outb_p(SECTOR_NUMBER_REGISTER, (uint8_t)(lba & 0xFF)); // Low
	outb_p(CYLINDER_LOW_REGISTER, (uint8_t)(lba >> 8));    // Mid
	outb_p(CYLINDER_HIGH_REGISTER, (uint8_t)(lba >> 16));  // High
	outb_p(COMMAND_REGISTER, 0x30); // Write

	uint16_t* ptr = (uint16_t*)buffer;
	for(uint8_t sector = 0; sector < totalSectors; sector++){
		t = TRIES;
		do {
			status = inb_p(STATUS_REGISTER);

			if (status & ATA_SR_ERR)
				return ERROR;
			if (!t--)
				return TIMEOUT;
		} while (!(status & ATA_SR_DRQ));

		outsw(DATA_RESGISTE, ptr, WORDS_PER_SECTOR);
		ptr += WORDS_PER_SECTOR;
	}

	// Flush
	outb_p(COMMAND_REGISTER, 0xE7);

	return SUCCESS;
}

int ata_write_sectors_48(uint64_t lba, uint16_t totalSectors, void* buffer){
	return NOT_IMPLEMENTED;
}

