#include <stdint.h>
#include <wey/vfs.h>
#include <def/err.h>
#include <lib/div64.h>
#include <io/ports.h>
#include <drivers/ata.h>

#include "ata_internal.h"

int ata_flush(struct ATADevice* atadev) {
	struct ATAChannel* ch = atadev->channel;
	atadev->irqTriggered = 0;

	outb_p(ATA_IO(ch, ATA_REG_HDDEVSEL), 0xE0 | (atadev->drive << 4));
	outb_p(ATA_IO(ch, ATA_REG_COMMAND), atadev->info.supports_lba48 ? 
		ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH
	);

	ata_wait_irq(atadev);

	uint8_t status;

	while((status = ata_status(atadev)) & ATA_SR_BSY);

	if (status & ATA_SR_ERR){
		return -inb_p(ATA_IO(ch, ATA_REG_ERROR));
	}

	return SUCCESS;
}

static int _pio_28_io_cmd(struct ATADevice* atadev, uint8_t cmd, uint32_t lba, uint8_t totalSectors){
	if(cmd != ATA_CMD_READ_PIO && cmd != ATA_CMD_WRITE_PIO){
		return INVALID_ARG;
	}

	struct ATAChannel* ch = atadev->channel;
	ch->irqRegistered = 0;

	inb(ATA_IO(ch, ATA_REG_STATUS));
	while (ata_status(atadev) & ATA_SR_BSY);

	outb_p(ATA_IO(ch, ATA_REG_HDDEVSEL), 0xE0 | (atadev->drive << 4) | ((lba >> 24) & 0x0F));
	outb_p(ATA_IO(ch, ATA_REG_SECCOUNT0), totalSectors);

	outb_p(ATA_IO(ch, ATA_REG_LBA0), (uint8_t)(lba));
	outb_p(ATA_IO(ch, ATA_REG_LBA1), (uint8_t)(lba >> 8));
	outb_p(ATA_IO(ch, ATA_REG_LBA2), (uint8_t)(lba >> 16));

	outb_p(ATA_IO(ch, ATA_REG_COMMAND), cmd);

	return SUCCESS;
}

static int _pio_48_io_cmd(struct ATADevice* atadev, uint8_t cmd, uint64_t lba, uint16_t totalSectors){
	if(cmd != ATA_CMD_READ_PIO_EXT && cmd != ATA_CMD_WRITE_PIO_EXT){
		return INVALID_ARG;
	}

	struct ATAChannel* ch = atadev->channel;
	ch->irqRegistered = 0;

	inb(ATA_IO(ch, ATA_REG_STATUS));
	while (ata_status(atadev) & ATA_SR_BSY);

	outb_p(ATA_IO(ch, ATA_REG_HDDEVSEL), 0x40 | (atadev->drive << 4));

	outb_p(ATA_IO(ch, ATA_REG_SECCOUNT0), (totalSectors >> 8) & 0xFF);
	outb_p(ATA_IO(ch, ATA_REG_LBA0), (lba >> 24) & 0xFF);
	outb_p(ATA_IO(ch, ATA_REG_LBA1), (lba >> 32) & 0xFF);
	outb_p(ATA_IO(ch, ATA_REG_LBA2), (lba >> 40) & 0xFF);

	outb_p(ATA_IO(ch, ATA_REG_SECCOUNT0), totalSectors & 0xFF);
	outb_p(ATA_IO(ch, ATA_REG_LBA0), (lba >> 0) & 0xFF);
	outb_p(ATA_IO(ch, ATA_REG_LBA1), (lba >> 8) & 0xFF);
	outb_p(ATA_IO(ch, ATA_REG_LBA2), (lba >> 16) & 0xFF);

	outb_p(ATA_IO(ch, ATA_REG_COMMAND), cmd);

	return SUCCESS;
}

static int _pio_read(struct ATADevice* atadev, void* buffer, int totalSectors){	
	uint16_t* ptr = (uint16_t*)buffer;
	struct ATAChannel* ch = atadev->channel;
	int words = atadev->info.sec_size / 2;
	uint8_t status;

	for(int sector = 0; sector < totalSectors; sector++){
		// Wait BSY clear
		while((status = ata_status(atadev)) & ATA_SR_BSY);

		if (status & ATA_SR_ERR)
			return -inb_p(ATA_IO(ch, ATA_REG_ERROR));

		// Wait DRQ set
		while (!(status & ATA_SR_DRQ)) {
			status = ata_status(atadev);

			if (status & ATA_SR_ERR)
				return -inb_p(ATA_IO(ch, ATA_REG_ERROR));
		}

		insw(ATA_IO(ch, ATA_REG_DATA), ptr, words);
		ptr += words;
	}

	return SUCCESS;
}

static int _pio_write(struct ATADevice* atadev, const void* buffer, int totalSectors){
	const uint16_t* ptr = (uint16_t*)buffer;
	struct ATAChannel* ch = atadev->channel;
	int words = atadev->info.sec_size / 2;
	uint8_t status;
	
	for(int sector = 0; sector < totalSectors; sector++){
		// Wait BSY clear
		while((status = ata_status(atadev)) & ATA_SR_BSY);

		if (status & ATA_SR_ERR)
			return -inb_p(ATA_IO(ch, ATA_REG_ERROR));

		// Wait DRQ set
		while (!(status & ATA_SR_DRQ)) {
			status = ata_status(atadev);

			if (status & ATA_SR_ERR)
				return -inb_p(ATA_IO(ch, ATA_REG_ERROR));
		}

		for (int i = 0; i < words; i++) {
			outw_p(ATA_IO(ch, ATA_REG_DATA), *ptr++);
		}
	}

	return ata_flush(atadev);
}

int ata_pio(struct ATADevice* atadev, uint8_t cmd_pio, sector_t sector, unsigned int seccount, void* buffer) {
	if(!atadev->exists){
		return INVALID_ARG;
	}

	if(seccount > 0xFFFF){
		return OUT_OF_BOUNDS;
	}

	int ret = OUT_OF_BOUNDS;
	const int PIO28_MAX_LBA = 0x0FFFFFFF;
	if(sector > PIO28_MAX_LBA && atadev->info.supports_lba48){
		seccount &= 0xFFFF;
		ret = _pio_48_io_cmd(atadev, cmd_pio, sector, seccount);
	}else if(sector <= PIO28_MAX_LBA){
		if (seccount > 256)
			goto out;

		seccount &= 0xFF;

		cmd_pio -= 0x4; // convert 48 -> 28 PIO
		ret = _pio_28_io_cmd(atadev, cmd_pio, sector, seccount);
	}

	if(IS_ERR_VALUE(ret)){
		goto out;
	}

	while (ata_status(atadev) & ATA_SR_BSY);
	ret = ata_wait_irq(atadev);

	if(IS_ERR_VALUE(ret)){
		goto out;
	}

	if (cmd_pio == ATA_CMD_WRITE_PIO || cmd_pio == ATA_CMD_WRITE_PIO_EXT)
		ret = _pio_write(atadev, buffer, seccount);
	else
		ret = _pio_read(atadev, buffer, seccount);

out:
	return ret;
}
