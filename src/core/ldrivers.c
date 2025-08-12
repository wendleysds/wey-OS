#include <drivers/ata.h>
#include <drivers/fat_fs.h>

void load_drivers(){
    ata_init();
    fat_fs_init();
}
