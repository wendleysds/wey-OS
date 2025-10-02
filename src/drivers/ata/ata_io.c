#include <fs/vfs.h>
#include <def/err.h>
#include <io/ports.h>
#include <core/sched/task.h>
#include <drivers/ata.h>

#include "ata_internal.h"

int ata_flush(struct ATADevice* atadev) {
	struct ATAChannel* ch = atadev->channel;
	ch->active = atadev;
	atadev->irqTriggered = 0;

	outb_p(ATA_IO(ch, ATA_REG_HDDEVSEL), 0xE0 | (atadev->drive << 4));
	outb_p(ATA_IO(ch, ATA_REG_COMMAND), atadev->info.isLBA48 ? 
		ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH
	);

	ata_wait_irq(atadev);

	if (ata_status(atadev) & ATA_SR_ERR)
		return -inb_p(ATA_IO(ch, ATA_REG_ERROR));

	return SUCCESS;
}

static int _pio_28_io_cmd(struct ATADevice* atadev, uint8_t cmd, uint32_t lba, uint8_t totalSectors){
	if(cmd != ATA_CMD_READ_PIO && cmd != ATA_CMD_WRITE_PIO){
		return INVALID_ARG;
	}

	struct ATAChannel* ch = atadev->channel;

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

	for(int sector = 0; sector < totalSectors; sector++){
		ata_wait_irq(atadev);

		if (ata_status(atadev) & ATA_SR_ERR){
			return -inb_p(ATA_IO(ch, ATA_REG_ERROR));
		}

		insw(ATA_IO(ch, ATA_REG_DATA), ptr, WORDS_PER_SECTOR);
		ptr += WORDS_PER_SECTOR;

		atadev->irqTriggered = 0;
	}

	return SUCCESS;
}

static int _pio_write(struct ATADevice* atadev, const void* buffer, int totalSectors){
	uint16_t* ptr = (uint16_t*)buffer;
	struct ATAChannel* ch = atadev->channel;
	
	for(int sector = 0; sector < totalSectors; sector++){
		ata_wait_irq(atadev);

		if (ata_status(atadev) & ATA_SR_ERR){
			return -inb_p(ATA_IO(ch, ATA_REG_ERROR));
		}

		for (int i = 0; i < WORDS_PER_SECTOR; i++) {
			outw_p(ATA_IO(ch, ATA_REG_DATA), *ptr++);
		}

		atadev->irqTriggered = 0;
	}

	return ata_flush(atadev);
}

static int _ata_pio_common(struct file *file, void *buffer, uint32_t count, uint8_t cmd_pio, uint8_t cmd_pio_ext, int is_write) {
	struct ATADevice* atadev = (struct ATADevice*)file->private_data;
	if (!atadev || count == 0 || !buffer) {
		return INVALID_ARG;
	}

	if(!atadev->exists){
		return INVALID_ARG;
	}

	if (count % SECTOR_SIZE != 0) {
		return BAD_ALIGNMENT;
	}

	uint64_t pos = (file->pos &= ~(SECTOR_SIZE - 1));

	uint64_t lba = pos / SECTOR_SIZE;
	uint16_t totalSectors = count / SECTOR_SIZE;

	atadev->channel->active = atadev;
	atadev->irqTriggered = 0;

	int ret;
	if (lba > 0x0FFFFFFF && atadev->info.isLBA48) {
		ret = _pio_48_io_cmd(atadev, cmd_pio_ext, lba, totalSectors);
	} else if (lba < 0x0FFFFFFF && totalSectors < 0xFF) {
		ret = _pio_28_io_cmd(atadev, cmd_pio, lba, totalSectors);
	} else {
		return OUT_OF_BOUNDS;
	}

	if (IS_STAT_ERR(ret)) {
		return ret;
	}

	if (is_write)
		return _pio_write(atadev, buffer, totalSectors);
	else
		return _pio_read(atadev, buffer, totalSectors);
}

int ata_read(struct file *file, void *buffer, uint32_t count) {
	return _ata_pio_common(file, buffer, count, ATA_CMD_READ_PIO, ATA_CMD_READ_PIO_EXT, 0);
}

int ata_write(struct file *file, const void *buffer, uint32_t count) {
	// Cast away const for compatibility with _pio_write signature
	return _ata_pio_common(file, (void*)buffer, count, ATA_CMD_WRITE_PIO, ATA_CMD_WRITE_PIO_EXT, 1);
}

int ata_lseek(struct file *file, int offset, int whence){
	struct ATADevice* atadev = (struct ATADevice*)file->private_data;
	if (!atadev)
		return INVALID_ARG;

	uint64_t drive_size = atadev->addressableSectors * SECTOR_SIZE;

	uint64_t new_pos;
	switch (whence) {
		case SEEK_SET:
			new_pos = offset;
			break;
		case SEEK_CUR:
			new_pos = file->pos + offset;
			break;
		case SEEK_END:
			new_pos = drive_size + offset;
			break;
		default:
			return INVALID_ARG;
	}

	if (new_pos > drive_size || new_pos < 0)
		return OUT_OF_BOUNDS;

	file->pos = new_pos;
	return SUCCESS;
}
