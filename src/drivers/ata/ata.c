#include <drivers/ata.h>
#include <io/ports.h>
#include <def/status.h>
#include <arch/i386/pic.h>
#include <arch/i386/idt.h>
#include <core/sched.h>
#include <core/kernel.h>
#include <lib/string.h>
#include <device.h>

#define SECTOR_SIZE 512

#define ATA_PRIMARY_IRQ   14
#define ATA_SECONDARY_IRQ 15

static struct ATADevice ata_devices[4];
static volatile int8_t ata_irq_invoked = 0;

static void _ata_irq_handler(struct InterruptFrame* frame){
	ata_irq_invoked = 1;
}

static void _ata_wait_irq(int channel) {
    while (!ata_irq_invoked) {
        // Sleep task
		__asm__ volatile ("hlt");
    }

    ata_irq_invoked = 0;
}

static inline int8_t _ata_polling(struct ATADevice* dev){
	int i;
	for (i = 0; i < TRIES; i++)
	{
		uint8_t status = inb_p(ATA_IO(dev, ATA_REG_STATUS));
		if (status & ATA_SR_ERR) return -inb_p(ATA_IO(dev, ATA_REG_ERROR));
		if (status & ATA_SR_DRQ) return 0;
	}

	return TIMEOUT;
}

static inline uint8_t _is_ata(struct device* dev){
	if(dev->type != DEVICE_BLOCK){
		return 0;
	}
	
	for (int i = 0; i < 4; i++)
	{
		if(dev->driver_data == &ata_devices[i]){
			return 1;
		}
	}

	return 0;
}

static inline int ata_flush(struct ATADevice* dev, uint8_t lba48) {
	int channel = (dev->ioBase == 0x1F0) ? 0 : 1;

	outb_p(ATA_IO(dev, ATA_REG_HDDEVSEL), 0xE0 | (dev->slave << 4));
	outb_p(ATA_IO(dev, ATA_REG_COMMAND), lba48 ? ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH);

	int i;
	for (i = TRIES; i > 0; i--) {
		uint8_t status = inb_p(ATA_IO(dev, ATA_REG_STATUS));
		if (!(status & ATA_SR_BSY)) break;
	}

	if (i <= 0) return TIMEOUT;

	_ata_wait_irq(channel);

	uint8_t status = inb_p(ATA_IO(dev, ATA_REG_STATUS));
	if (status & ATA_SR_ERR)
		return -inb_p(ATA_IO(dev, ATA_REG_ERROR));

	return SUCCESS;
}

static void _ata_register_irq(uint8_t channel){
	uint8_t irq = (channel == 0 ? ATA_PRIMARY_IRQ : ATA_SECONDARY_IRQ);

	idt_register_callback(IRQ(irq), _ata_irq_handler);
	IRQ_clear_mask(irq);
}

static void _ata_probe_all() {
	uint16_t ioBases[] = { 0x1F0, 0x170 };
	uint16_t ctrlBases[] = { 0x3F6, 0x376 };

	for (int channel = 0; channel < 2; channel++) {
		for (int drive = 0; drive < 2; drive++) {
			struct ATADevice* atadev = &ata_devices[channel * 2 + drive];
			atadev->ioBase = ioBases[channel];
			atadev->ctrlBase = ctrlBases[channel];
			atadev->slave = drive;

			uint16_t buffer[WORDS_PER_SECTOR];
			if (ata_identify(atadev, buffer) == SUCCESS) {
				_ata_register_irq(channel);

				atadev->exists = 1;
				atadev->isHD = buffer[0];
				atadev->UDMAmodes = buffer[88];

				atadev->lba48 = (buffer[83] & (1 << 10)) != 0;

				if (atadev->lba48) {
					atadev->addressableSectors = 
					((uint64_t)buffer[100] |
					((uint64_t)buffer[101] << 16) |
					((uint64_t)buffer[102] << 32) |
					((uint64_t)buffer[103] << 48));
				} else {
					atadev->addressableSectors = 
					((uint32_t)buffer[60] |
					((uint32_t)buffer[61] << 16));
				}

				uint8_t* dst = atadev->model;
				for (int i = 0; i < 20; i++) {
					uint16_t w = buffer[27 + i];
					*dst++ = w >> 8;
					*dst++ = w & 0xFF;
				}
				*dst = 0;

				struct device* dev = (struct device*)kmalloc(sizeof(struct device));
				if(!dev){
					panic("_ata_probe_all(): dev malloc failed!");
				}

				struct device devbuffer = {
					.id = -1,
					.type = DEVICE_BLOCK,

					.mode = 0,
					.flags = 0,

					.driver_data = (void*)atadev,

					.read = ata_read,
					.write = ata_write,
					.ioctl = ata_ioctl,
					.close = ata_close
				};

				const char* names[] = {"hda", "hdb", "hdc", "hdd"};
				strcpy(devbuffer.name, names[channel * 2 + drive]);
				memcpy(dev, &devbuffer, sizeof(devbuffer));

				if(device_register(dev) == OUT_OF_BOUNDS){
					warning("Failed to register '%s'!\n", dev->name);
					kfree(dev);
				}

			} else {
				atadev->exists = 0;
			}
		}
	}
}

void ata_init(){
	memset(&ata_devices, 0x0, sizeof(ata_devices));
	ata_irq_invoked = 0;

	_ata_probe_all();
}

int ata_identify(struct ATADevice* dev, uint16_t* buffer) {
	outb_p(ATA_IO(dev, ATA_REG_HDDEVSEL), 0xA0 | (dev->slave << 4)); // Drive/head register
	outb_p(ATA_IO(dev, ATA_REG_SECCOUNT0), 0);
	outb_p(ATA_IO(dev, ATA_REG_LBA0), 0);
	outb_p(ATA_IO(dev, ATA_REG_LBA1), 0);
	outb_p(ATA_IO(dev, ATA_REG_LBA2), 0);
	outb_p(ATA_IO(dev, ATA_REG_COMMAND), ATA_CMD_IDENTIFY);

	int t = TRIES;
	uint8_t status;
	do {
		status = inb(ATA_IO(dev, ATA_REG_STATUS));
		if (status == 0) return INVALID_ARG;
        else if (status & ATA_SR_ERR) return ERROR;
        else if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break;
	} while (--t > 0);

	if (t <= 0) return TIMEOUT;

	uint8_t cl = inb_p(ATA_IO(dev, ATA_REG_LBA1));
	uint8_t ch = inb_p(ATA_IO(dev, ATA_REG_LBA2));

	if (cl != 0 || ch != 0) return OP_ABORTED;

    insw(ATA_IO(dev, ATA_REG_DATA), buffer, WORDS_PER_SECTOR);
    return SUCCESS;
}

static inline int ata_read_28(struct ATADevice* dev, void* buffer, uint32_t lba, uint8_t totalSectors){
	int channel = (dev->ioBase == 0x1F0) ? 0 : 1;
	uint32_t count = 0; 

	outb_p(ATA_IO(dev, ATA_REG_HDDEVSEL), 0xE0 | (dev->slave << 4) | ((lba >> 24) & 0x0F));
	outb_p(ATA_IO(dev, ATA_REG_SECCOUNT0), totalSectors);

	outb_p(ATA_IO(dev, ATA_REG_LBA0), (uint8_t)(lba));
	outb_p(ATA_IO(dev, ATA_REG_LBA1), (uint8_t)(lba >> 8));
	outb_p(ATA_IO(dev, ATA_REG_LBA2), (uint8_t)(lba >> 16));

	outb_p(ATA_IO(dev, ATA_REG_COMMAND), ATA_CMD_READ_PIO);

	int i;
	for (i = TRIES; i > 0; i--) {
		if (!(inb_p(ATA_IO(dev, ATA_REG_STATUS)) & ATA_SR_BSY)){
			break;
		}
	}

	if (i <= 0) return TIMEOUT;

	uint16_t* ptr = (uint16_t*)buffer;
	for(uint8_t sector = 0; sector < totalSectors; sector++){
		_ata_wait_irq(channel);
		uint8_t status = inb_p(ATA_IO(dev, ATA_REG_STATUS));
		if (status & ATA_SR_ERR) return -inb_p(ATA_IO(dev, ATA_REG_ERROR));

		insw(ATA_IO(dev, ATA_REG_DATA), ptr, WORDS_PER_SECTOR);
		ptr += WORDS_PER_SECTOR;
		count += SECTOR_SIZE;
	}

	return count;
}

static inline int ata_read_48(struct ATADevice* dev, void* buffer, uint64_t lba, uint64_t totalSectors){
	int channel = (dev->ioBase == 0x1F0) ? 0 : 1;
	uint64_t count = 0;

	outb_p(ATA_IO(dev, ATA_REG_SECCOUNT0), (totalSectors >> 8) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA0), (lba >> 24) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA1), (lba >> 32) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA2), (lba >> 40) & 0xFF);

	outb_p(ATA_IO(dev, ATA_REG_SECCOUNT0), totalSectors & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA0), (lba >> 0) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA1), (lba >> 8) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA2), (lba >> 16) & 0xFF);

	outb_p(ATA_IO(dev, ATA_REG_HDDEVSEL), 0x40 | (dev->slave << 4));
	outb_p(ATA_IO(dev, ATA_REG_COMMAND), ATA_CMD_READ_PIO_EXT);

	int i;
	for (i = TRIES; i > 0; i--) {
		if (!(inb_p(ATA_IO(dev, ATA_REG_STATUS)) & ATA_SR_BSY)){
			break;
		}
	}

	if (i <= 0) return TIMEOUT;

	uint16_t* ptr = (uint16_t*)buffer;
	for(uint64_t sector = 0; sector < totalSectors; sector++){
		_ata_wait_irq(channel);
		uint8_t status = inb_p(ATA_IO(dev, ATA_REG_STATUS));
		if (status & ATA_SR_ERR) return -inb_p(ATA_IO(dev, ATA_REG_ERROR));

		insw(ATA_IO(dev, ATA_REG_DATA), ptr, WORDS_PER_SECTOR);
		ptr += WORDS_PER_SECTOR;
		count += SECTOR_SIZE;
	}

	return count;
}

static inline int ata_write_28(struct ATADevice* dev, const void* buffer, uint32_t lba, uint8_t totalSectors){
	//int channel = (dev->ioBase == 0x1F0) ? 0 : 1;
	uint32_t count = 0; 

	outb_p(ATA_IO(dev, ATA_REG_HDDEVSEL), 0xE0 | (dev->slave << 4) | ((lba >> 24) & 0x0F));
	outb_p(ATA_IO(dev, ATA_REG_SECCOUNT0), totalSectors);

	outb_p(ATA_IO(dev, ATA_REG_LBA0), (uint8_t)(lba));
	outb_p(ATA_IO(dev, ATA_REG_LBA1), (uint8_t)(lba >> 8));
	outb_p(ATA_IO(dev, ATA_REG_LBA2), (uint8_t)(lba >> 16));

	outb_p(ATA_IO(dev, ATA_REG_COMMAND), ATA_CMD_WRITE_PIO);

	int i;
	for (i = TRIES; i > 0; i--) {
		if (!(inb_p(ATA_IO(dev, ATA_REG_STATUS)) & ATA_SR_BSY)){
			break;
		}
	}

	if (i <= 0) return TIMEOUT;

	uint16_t* ptr = (uint16_t*)buffer;
	for(uint8_t sector = 0; sector < totalSectors; sector++){
		uint8_t status;
		if((status = _ata_polling(dev)) != 0){
			return status;
		}
			
		status = inb_p(ATA_IO(dev, ATA_REG_STATUS));
		if (status & ATA_SR_ERR) return -inb_p(ATA_IO(dev, ATA_REG_ERROR));

		for (int i = 0; i < WORDS_PER_SECTOR; i++) {
			outw_p(ATA_IO(dev, ATA_REG_DATA), *ptr++);
			count++;
		}
	}

	ata_flush(dev, 0);

	return count;
}

static inline int ata_write_48(struct ATADevice* dev, const void* buffer, uint64_t lba, uint16_t totalSectors){
	//int channel = (dev->ioBase == 0x1F0) ? 0 : 1;
	uint64_t count = 0;

	outb_p(ATA_IO(dev, ATA_REG_SECCOUNT0), (totalSectors >> 8) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA0), (lba >> 24) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA1), (lba >> 32) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA2), (lba >> 40) & 0xFF);

	outb_p(ATA_IO(dev, ATA_REG_SECCOUNT0), totalSectors & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA0), (lba >> 0) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA1), (lba >> 8) & 0xFF);
	outb_p(ATA_IO(dev, ATA_REG_LBA2), (lba >> 16) & 0xFF);

	outb_p(ATA_IO(dev, ATA_REG_HDDEVSEL), 0x40 | (dev->slave << 4));
	outb_p(ATA_IO(dev, ATA_REG_COMMAND), ATA_CMD_WRITE_PIO_EXT);

	int i;
	for (i = TRIES; i > 0; i--) {
		if (!(inb_p(ATA_IO(dev, ATA_REG_STATUS)) & ATA_SR_BSY)){
			break;
		}
	}

	if (i <= 0) return TIMEOUT;

	uint16_t* ptr = (uint16_t*)buffer;
	for(uint8_t sector = 0; sector < totalSectors; sector++){
		uint8_t status;
		if((status = _ata_polling(dev)) != 0){
			return status;
		}	

		status = inb_p(ATA_IO(dev, ATA_REG_STATUS));
		if (status & ATA_SR_ERR) return -inb_p(ATA_IO(dev, ATA_REG_ERROR));

		for (int i = 0; i < WORDS_PER_SECTOR; i++) {
			outw_p(ATA_IO(dev, ATA_REG_DATA), *ptr++);
			count++;
		}
	}

	ata_flush(dev, 1);

	return count;
}

int ata_read(struct device* dev, void* buffer, uint32_t size, uint64_t offset){
	if(!dev || offset < 0 || size <= 0 || !buffer){
		return INVALID_ARG;
	}

	if(!_is_ata(dev)){
		return INVALID_ARG;
	}

	if (offset % SECTOR_SIZE != 0 || size % SECTOR_SIZE != 0){
		return BAD_ALIGNMENT;
	}

	uint32_t lba = offset / SECTOR_SIZE;
	uint32_t totalSectors = size / SECTOR_SIZE;

	struct ATADevice* atadev = (struct ATADevice*)dev->driver_data;

	if(lba >= 0x10000000){
		if(atadev->lba48)
			return ata_read_48(atadev, buffer, lba, totalSectors);
		return OUT_OF_BOUNDS;
	}

	if(size < 0xFF)
		return OUT_OF_BOUNDS;

	return ata_read_28(atadev, buffer, lba, totalSectors);
}

int ata_write(struct device* dev, const void* buffer, uint32_t size, uint64_t offset){
	if(!dev || offset < 0 || size <= 0 || !buffer){
		return INVALID_ARG;
	}

	if(!_is_ata(dev)){
		return INVALID_ARG;
	}

	if (offset % SECTOR_SIZE != 0 || size % SECTOR_SIZE != 0){
		return BAD_ALIGNMENT;
	}

	uint32_t lba = offset / SECTOR_SIZE;
	uint32_t totalSectors = size / SECTOR_SIZE;

	struct ATADevice* atadev = (struct ATADevice*)dev->driver_data;

	if(lba >= 0x10000000){
		if(atadev->lba48 && size < 0xFFFF)
			return ata_write_48(atadev, buffer, lba, totalSectors);
		return OUT_OF_BOUNDS;
	}

	if(size < 0xFF)
		return OUT_OF_BOUNDS;

	return ata_write_28(atadev, buffer, lba, totalSectors);
}

long ata_ioctl(struct device* dev, unsigned int cmd, unsigned long arg){
	return NOT_IMPLEMENTED;
}

int ata_close(struct device* dev){
	return NOT_IMPLEMENTED;
}
