#include <drivers/ata.h>
#include <drivers/fat_fs.h>
#include <drivers/keyboard.h>

void load_drivers(){
    ata_init();
    fat_fs_init();
    keyboard_init();
}
