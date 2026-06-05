#include <device/ata.h>
#include <io/ports.h>
#include <def/err.h>

#include "ata_internal.h"

int ata_identify(struct ATADevice* atadev, uint16_t* buffer) {
	struct ATAChannel* channel = atadev->channel;
	channel->active = atadev;

	outb_p(ATA_IO(channel, ATA_REG_HDDEVSEL), 0xA0 | (atadev->drive << 4)); // Drive/head register
	outb_p(ATA_IO(channel, ATA_REG_SECCOUNT0), 0);
	outb_p(ATA_IO(channel, ATA_REG_LBA0), 0);
	outb_p(ATA_IO(channel, ATA_REG_LBA1), 0);
	outb_p(ATA_IO(channel, ATA_REG_LBA2), 0);
	outb_p(ATA_IO(channel, ATA_REG_COMMAND), ATA_CMD_IDENTIFY);

	uint8_t status;
	if(IS_STAT_ERR(status = ata_wait_irq(atadev))){
		return status;
	}

	uint8_t cl = inb_p(ATA_IO(channel, ATA_REG_LBA1));
	uint8_t ch = inb_p(ATA_IO(channel, ATA_REG_LBA2));

	if (cl != 0 || ch != 0) return INVALID_STATE;

	insw(ATA_IO(channel, ATA_REG_DATA), buffer, WORDS_PER_SECTOR);
	return SUCCESS;
}
