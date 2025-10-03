#include "vfat_fs_internal.h"
#include <fs/vfs.h>
#include <def/err.h>
#include <mmu.h>
#include <io/stream.h>
#include <lib/string.h>

struct inode_operations vfat_fs_iop = {
    .lookup = fat_lookup,
    .create = fat_create,
    .unlink = fat_unlink,
    .mkdir = fat_mkdir,
    .rmdir = fat_rmdir,
    .getattr = fat_getattr,
    .setarrt = fat_setarrt
};

struct file_operations vfat_fs_fop = {
    .read = fat_read,
    .write = fat_write,
    .lseek = fat_lseek,
    .close = fat_close
};

static int8_t _valid_fat_sector(const uint8_t* sector0){
    if (sector0[0] != 0xEB && sector0[0] != 0xE9) {
        return 0;
    }

    if (sector0[510] != 0x55 || sector0[511] != 0xAA){
        return 0;
    }

    int valid_oem = 1;
    for (int i = 3; i < 11; ++i) {
        if (sector0[i] < 0x20 || sector0[i] > 0x7E) {
            valid_oem = 0;
            break;
        }
    }

    return valid_oem;
}

static fat_type_t _fat_get_type(const uint8_t* sector0Buffer){
    const struct FATHeader* bpb = (const struct FATHeader*)sector0Buffer;
    const struct FAT32HeaderExtended* fat32ext = (const struct FAT32HeaderExtended*)(sector0Buffer + sizeof( struct FATHeader));

    if (bpb->bytesPerSec == 0 || bpb->secPerClus == 0){
        return FAT_INVALID;
    }

    uint32_t rootDirSectors = ((bpb->rootEntCnt * 32) + (bpb->bytesPerSec - 1)) / bpb->bytesPerSec;

    uint32_t totSec = (bpb->totSec16 != 0) ? bpb->totSec16 : bpb->totSec32;
    uint32_t fatSz = (bpb->FATSz16 != 0) ? bpb->FATSz16 : fat32ext->FATSz32;

    uint32_t dataSec = totSec - (bpb->rsvdSecCnt + (bpb->numFATs * fatSz) + rootDirSectors);

    uint32_t countOfClusters = dataSec / bpb->secPerClus;

    if (countOfClusters < 4085)
        return FAT_TYPE_12;
    else if (countOfClusters < 65525)
        if(bpb->FATSz16 != 0)
            return FAT_TYPE_16;
        else
            return FAT_TYPE_32;
    else
        return FAT_TYPE_32;
}

static struct FAT* fat_init(struct blkdev* bdev){
	struct Stream* stream = stream_new(bdev);
    if(!stream){
        return ERR_PTR(NO_MEMORY);
    }

    struct FAT* fat = (struct FAT*)kmalloc(sizeof(struct FAT));
    if(!fat){
        stream_dispose(stream);
        return ERR_PTR(NO_MEMORY);
    }

    int8_t res;
    uint8_t sector0[SECTOR_SIZE];
    if((res = stream_read(stream, sector0, sizeof(sector0))) != SUCCESS){
        goto err;
    }

    if(!_valid_fat_sector(sector0)){
        res = INVALID_FS;
        goto err;
    }

    fat_type_t type = _fat_get_type(sector0);
    if (type >= FAT_TYPES_COUNT || type < 0 || type == FAT_INVALID) {
        res = INVALID_FS;
        goto err;
    }

    fat->type = type;
    if((res = loaders[type](fat, stream, sector0)) != SUCCESS){
        goto err;
    }

    fat->stream = stream;

    return fat;

err:
    kfree(fat);
    stream_dispose(stream);
    return ERR_PTR(res);
}

static struct inode* fat_mount(struct file_system_type* fs_type, int flags, const char* dev_name, void* data){
    if(!fs_type || !dev_name){
        return ERR_PTR(INVALID_ARG);
    }

	struct blkdev* bdev = blkdev_find_by_name(dev_name);
	if(IS_ERR(bdev)){
		return ERR_CAST(bdev);
	}

	struct super_block* fat_sb = (struct super_block*)kzalloc(sizeof(struct super_block));
	if(!fat_sb){
		return ERR_PTR(NO_MEMORY);
	}

    struct inode* fat_root = (struct inode*)kmalloc(sizeof(struct inode));
    if(!fat_root){
		kfree(fat_sb);
        return ERR_PTR(NO_MEMORY);
    }

    struct FATFileDescriptor* root_fd = (struct FATFileDescriptor*)kmalloc(sizeof(struct FATFileDescriptor));
    if(!root_fd){
        kfree(fat_sb);
        kfree(fat_root);
        return ERR_PTR(NO_MEMORY);
    }

    struct FAT* fat = fat_init(bdev);
    if(IS_ERR(fat)){
        kfree(fat_sb);
        kfree(fat_root);
        kfree(root_fd);
        return ERR_CAST(fat);
    }
    
    root_fd->fat = fat;
    root_fd->firstCluster = fat->rootClus;
    root_fd->currentCluster = root_fd->firstCluster;
    root_fd->parentDirCluster = root_fd->firstCluster;

    memset(&root_fd->entry, 0, sizeof(root_fd->entry));
    memcpy(&root_fd->entry, "ROOT       ", 11);
    root_fd->entry.attr = ATTR_DIRECTORY | ATTR_SYSTEM;
    root_fd->entry.fstClusHI = root_fd->firstCluster >> 16;
    root_fd->entry.fstClusLO = root_fd->firstCluster & 0xFFFF;

    fat_root->i_op = &vfat_fs_iop;
    fat_root->i_fop = &vfat_fs_fop;
    fat_root->size = 0;
    fat_root->mode = S_IFDIR;
    fat_root->private_data = (void*)root_fd;
	fat_root->i_sb = fat_sb;

    fat_sb->root_inode = fat_root;
    fat_sb->private_data = (void*)fat;
	fat_sb->bdev = bdev;
	fat_sb->fs_type = fs_type;

    return fat_root;
}

static int fat_unmount(struct super_block* sb){
    if(!sb){
        return INVALID_ARG;
    }

    if(sb->private_data){
        struct FAT* fat = (struct FAT*)sb->private_data;

        switch (fat->type)
        {
        case FAT_TYPE_12:
            kfree(fat->table.fat12);
            break;
        case FAT_TYPE_16:
            kfree(fat->table.fat16);
            break;
        case FAT_TYPE_32:
            kfree(fat->table.fat32);
            break;
        default:
            return INVALID_FS;
        }

        stream_dispose(fat->stream);
        kfree(fat);
    }

    return SUCCESS;
}

static struct file_system_type fat_fs = {
    .name = "vfat",
    .mount = fat_mount,
    .unmount = fat_unmount
};

void fat_fs_init(){
    vfs_register_filesystem(&fat_fs);
}
