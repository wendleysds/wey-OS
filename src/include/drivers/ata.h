#ifndef _ATA_LBA_H
#define _ATA_LBA_H

#include <device.h>
#include <stdint.h>

#define TRIES 100000
#define WORDS_PER_SECTOR 256

// ATA Registers
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

// ATA Status
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

// ATA Comands

#define ATA_CMD_READ_PIO         0x20
#define ATA_CMD_READ_PIO_EXT     0x24
#define ATA_CMD_READ_DMA         0xC8
#define ATA_CMD_READ_DMA_EXT     0x25
#define ATA_CMD_WRITE_PIO        0x30
#define ATA_CMD_WRITE_PIO_EXT    0x34
#define ATA_CMD_WRITE_DMA        0xCA
#define ATA_CMD_WRITE_DMA_EXT    0x35
#define ATA_CMD_CACHE_FLUSH      0xE7
#define ATA_CMD_CACHE_FLUSH_EXT  0xEA
#define ATA_CMD_PACKET           0xA0
#define ATA_CMD_IDENTIFY_PACKET  0xA1
#define ATA_CMD_IDENTIFY         0xEC

#define ATAPI_CMD_READ           0xA8
#define ATAPI_CMD_EJECT          0x1B

// ATA Errors

#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

// ATA io port
#define ATA_IO(atadev, reg) ((atadev)->ioBase + (reg))

struct ATADevice {
    uint8_t exists;
    uint16_t ioBase;    // 0x1F0 or 0x170
    uint16_t ctrlBase;  // 0x3F6 or 0x376
    uint8_t slave;      // 0 = master, 1 = slave 
    uint8_t isHD;
    uint8_t lba48;
    uint16_t UDMAmodes;
    uint64_t addressableSectors;
    uint8_t model[41];
};

void ata_init();

int ata_identify(struct ATADevice* dev, uint16_t* buffer);

int ata_read(struct device* dev, void* buffer, uint32_t size, uint64_t offset);
int ata_write(struct device* dev, const void* buffer, uint32_t size, uint64_t offset);
long ata_ioctl(struct device* dev, unsigned int cmd, unsigned long arg);
int ata_close(struct device* dev);

#endif

