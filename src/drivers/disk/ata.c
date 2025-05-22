#include <drivers/ATA/ata.h>
#include <io.h>
#include <def/status.h>
#include <stdint.h>

#define TRIES 100000
#define WORDS_PER_READ 256

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
	// Wait until disk be ready
	uint8_t status = FAILED;
	for(int i = 0; i < TRIES; i++){
		uint8_t status = inb(STATUS_REGISTER);
		if(!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)){
			status = OK;
			break;
		}
	}

	if(status == FAILED)
		return TIMEOUT;

	outb(DRIVE_HEAD_REGISTER, (lba >> 24) | 0xE0);       // Drive + LBA bits
	outb(SECTOR_COUNT_REGISTER, totalSectors);
	outb(SECTOR_NUMBER_REGISTER, (uint8_t)(lba & 0xFF)); // Low
	outb(CYLINDER_LOW_REGISTER, (uint8_t)(lba >> 8));    // Mid
	outb(CYLINDER_HIGH_REGISTER, (uint8_t)(lba >> 16));  // High
	outb(COMMAND_REGISTER, 0x20); // Read
	
	uint16_t* ptr = (uint16_t*)buffer;
	for(uint8_t sector = 0; sector < totalSectors; sector++){
		int tries = TRIES;
		while(tries--){
			status = inb(0x1F7);
			if (status & ATA_SR_ERR)
				return ERROR;

			if (status & ATA_SR_DRQ)
				break;
		}

		if(tries < 0)
			return TIMEOUT;

		for(int i = 0; i < WORDS_PER_READ; i++){
			*ptr++ = insw(0x1F0);
		}
	}

	return SUCCESS;
}

