#include <def/err.h>
#include <platform.h>
#include <string.h>
#include <stddef.h>
#include <utils.h>
#include <disk.h>
#include <heap.h>
#include "bios.h"

struct disk* main_disk;

struct bios_disk_data{
	struct{
		uint8_t sectorPerTrack;
		uint8_t numHeads;
		uint16_t numCylinders;
	} geo;

	uint8_t driveNum;
} __packed;

struct bios_dap{
	uint8_t size;
	uint8_t reserved;
	uint16_t seccount;
	uint16_t buffer_offset;
	uint16_t buffer_segment;
	uint64_t lba;
} __packed;

static int _disk_read_chs(struct disk* disk, uint64_t lba, void* buffer, uint16_t seccount){
	struct bios_disk_data* bdsk_data = (struct bios_disk_data*)disk->private;

	if (lba > 0xFFFFFFFFULL || seccount > 255) {
        return OUT_OF_BOUNDS;
    }

	uint32_t lba32 = (uint32_t)lba;
	uint32_t tmp = lba32 / bdsk_data->geo.sectorPerTrack;

	uint8_t sector = (lba32 % bdsk_data->geo.sectorPerTrack) + 1;
	uint8_t head = tmp % bdsk_data->geo.numHeads;
	uint16_t cylinder = tmp / bdsk_data->geo.numHeads;

	struct biosregs ireg, oreg;
	ireg.ah = 0x02; // read
    ireg.al = seccount;
    ireg.ch = cylinder & 0xFF;
    ireg.cl = sector | ((cylinder >> 2) & 0xC0);
    ireg.dh = head;
    ireg.dl = bdsk_data->driveNum;
    ireg.es = SEG(buffer);
    ireg.bx = OFF(buffer);

	intcall(0x13, &ireg, &oreg);

	if(oreg.eflags & X86_EFLAGS_CF){
		return READ_FAIL;
	}

	return SUCCESS;
}

static int _disk_read_dap(struct disk* disk, uint64_t lba, void* buffer, uint16_t seccount){
	struct bios_disk_data* bdsk_data = (struct bios_disk_data*)disk->private;

	struct bios_dap dap;
	dap.size = sizeof(struct bios_dap);
	dap.reserved = 0;
	dap.seccount = seccount;
	dap.buffer_offset = OFF(buffer);
	dap.buffer_segment = SEG(buffer);
	dap.lba = lba;

	struct biosregs ireg, oreg;
	initregs(&ireg);

	ireg.ah = 0x42;
	ireg.dl = bdsk_data->driveNum;
	ireg.si = (size_t)(&dap);

	intcall(0x13, &ireg, &oreg);

	if(oreg.eflags & X86_EFLAGS_CF){
		return READ_FAIL;
	}

	return SUCCESS;
}

static int _nop(struct disk* disk, uint64_t lba, const void* buffer, uint16_t seccount){
	return NOT_SUPPORTED;
}

int platform_disk_init(){
	struct biosregs ireg, oreg;
	initregs(&ireg);

	ireg.ah = 0x41;
	ireg.dl = mainDriverNum;
	intcall(0x13, &ireg, &oreg);

	uint8_t support_dap = 0;
	if(!(oreg.eflags & X86_EFLAGS_CF) && oreg.bx == 0xAA55){
		support_dap = 1;
	}

	initregs(&ireg);
	ireg.ah = 0x08;
	ireg.dl = mainDriverNum;
	intcall(0x13, &ireg, &oreg);

	if(oreg.eflags & X86_EFLAGS_CF){
		return DISK_NOT_READY;
	}

	struct bios_disk_data* bdsk_data = (struct bios_disk_data*)malloc(sizeof(struct bios_disk_data));
	struct disk* dsk = (struct disk*)malloc(sizeof(struct disk));
	struct disk_op* ops = (struct disk_op*)malloc(sizeof(struct disk_op));

	bdsk_data->driveNum = mainDriverNum;
	bdsk_data->geo.sectorPerTrack = oreg.cl & 0x3F;
	bdsk_data->geo.numHeads = ((oreg.dh & 0xFF)) + 1;
	bdsk_data->geo.numCylinders = (((oreg.ch & 0xC0) << 2) | (oreg.ch & 0xFF)) + 1;

	dsk->private = bdsk_data;

	ops->read = support_dap ? _disk_read_dap : _disk_read_chs;
	ops->write = _nop;

	dsk->ops = ops;
	main_disk = dsk;

	return SUCCESS;
}
