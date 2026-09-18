#include <device/ata.h>
#include <io/ports.h>
#include <def/errno.h>

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

	uint8_t status = inb_p(ATA_IO(channel, ATA_REG_STATUS));
	if (status == 0x00 || status == 0xFF) {
		channel->active = NULL;
		return -ENODEV;
	}

	int ret;
	if (IS_ERR_VALUE(ret = ata_polling(atadev))) {
		channel->active = NULL;
		return ret;
	}

	uint8_t cl = inb_p(ATA_IO(channel, ATA_REG_LBA1));
	uint8_t ch = inb_p(ATA_IO(channel, ATA_REG_LBA2));

	if (cl != 0 || ch != 0) {
		channel->active = NULL;
		return -EINVAL;
	}

	insw(ATA_IO(channel, ATA_REG_DATA), buffer, WORDS_PER_SECTOR);
	channel->active = NULL;
	return SUCCESS;
}
