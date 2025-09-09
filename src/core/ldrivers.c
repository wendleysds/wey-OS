#include <drivers/ata.h>
#include <drivers/fat_fs.h>
#include <drivers/keyboard.h>

#include <def/config.h>

extern struct device* devices[DEVICES_MAX];

void load_drivers(){
	for (int i = 0; i < DEVICES_MAX; i++){
		devices[i] = 0x0;
	}

    ata_init();
    fat_fs_init();
    keyboard_init();
}
