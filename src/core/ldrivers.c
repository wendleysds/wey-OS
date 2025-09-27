#include <drivers/ata.h>
#include <drivers/fat_fs.h>
#include <drivers/keyboard.h>

#include <device.h>
#include <def/config.h>

void load_drivers(){
	device_init();
	
    ata_init();
    fat_fs_init();
    keyboard_init();
}
