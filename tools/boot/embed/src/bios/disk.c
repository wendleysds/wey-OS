#include <def/status.h>
#include <disk.h>
#include "bios.h"

struct disk* main_disk;

struct bios_disk{
	uint16_t sectorPerTrack;
	uint16_t numHeads;

	uint8_t driveNum;
	uint8_t support_dap;
};

static int _disk_read_chs(struct disk* disk, uint32_t lba, void* buffer, uint8_t seccount){
	/*Temp = LBA / (Sectors per Track)
    Sector = (LBA % (Sectors per Track)) + 1
    Head = Temp % (Number of Heads)
    Cylinder = Temp / (Number of Heads)*/
	return NOT_IMPLEMENTED;
}

static int _disk_read_dap(struct disk* disk, uint32_t lba, void* buffer, uint8_t seccount){
	return NOT_IMPLEMENTED;
}

static int _nop(){
	return NOT_SUPPORTED;
}

static int disk_support_dap(){
	return NOT_IMPLEMENTED;
}

int disk_init(uint8_t mainDriveNum){
	return NOT_IMPLEMENTED;
}