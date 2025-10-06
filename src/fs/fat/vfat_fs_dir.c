#include "vfat_fs_internal.h"
#include <memory/kheap.h>
#include <def/err.h>
#include <lib/mem.h>

static inline int _fat_count_entries(struct FAT* fat, uint32_t dirCluster){
    if(dirCluster < 2){
        return INVALID_ARG;
    }

    struct Stream* stream = fat->stream;

    int count = 0;
    uint32_t cluster = dirCluster;
    while (!fat_is_eof(fat, dirCluster)) {
        uint32_t lba = fat_cluster_to_lba(fat, cluster);
        stream_seek(stream, _SEC(lba), SEEK_SET);

        for (uint32_t i = 0; i < (fat->clusterSize / sizeof(struct FATLegacyEntry)); i++) {
            struct FATLegacyEntry buffer;
            if (stream_read(stream, &buffer, sizeof(buffer)) != SUCCESS) {
                return ERROR_IO;
            }

            if (buffer.name[0] == 0x0) {
                return count;
            }

            if (buffer.name[0] == 0xE5) {
                continue;
            }

            if ((buffer.attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME){
                continue;
            }

            count++;
        }

        cluster = fat_next_cluster(fat, cluster);
    }

    return count;
}

struct inode* fat_lookup(struct inode *dir, const char *name){
    if(!dir || !name){
        return ERR_PTR(INVALID_ARG);
    }

    struct FATFileDescriptor* dirfd = (struct FATFileDescriptor*)dir->private_data;

    if(!(dirfd->entry.attr & ATTR_DIRECTORY)){
        return ERR_PTR(INVALID_FILE);
    }

    struct FAT* fat = dirfd->fat;
    struct Stream* stream = fat->stream;
    uint32_t cluster = dirfd->firstCluster; 

    struct FATLegacyEntry entry;
    memset(&entry, 0x0, sizeof(entry));

    char fname[12];
    memset(fname, 0x0, sizeof(fname));
    fat_name_generate_short(name, fname);

    while (1) {
        uint32_t lba = fat_cluster_to_lba(fat, cluster);
        stream_seek(stream, _SEC(lba), SEEK_SET);

        for (uint32_t i = 0; i < (fat->clusterSize / sizeof(entry)); ++i) {
            if (stream_read(stream, &entry, sizeof(entry)) != SUCCESS) {
                return ERR_PTR(ERROR_IO);
            }

            if (entry.name[0] == 0x0) {
                return ERR_PTR(FILE_NOT_FOUND);
            }

            if (entry.name[0] == 0xE5) {
                continue;
            }

            if (memcmp((char *)entry.name, fname, 11) != 0) {
                continue;
            }

			struct inode* inode = inode_new(dir->i_sb);
			if(!inode){
				return ERR_PTR(NO_MEMORY);
			}

            struct FATFileDescriptor* newfd = (struct FATFileDescriptor*)kmalloc(sizeof(struct FATFileDescriptor));
            if(!newfd){
                kfree(inode);
                return ERR_PTR(NO_MEMORY);
            }

            newfd->fat = fat;
            newfd->firstCluster = ((entry.fstClusHI << 16) | entry.fstClusLO);
            newfd->currentCluster = newfd->firstCluster;
            newfd->parentDirCluster = cluster;
            newfd->entry = entry;

            inode->mode = (entry.attr & ATTR_DIRECTORY) ? S_IFDIR : S_IFREG;
            inode->size = entry.fileSize;
            inode->private_data = (void*)newfd;

            inode->i_op = &vfat_fs_iop;
            inode->i_fop = &vfat_fs_fop;

            return inode;
        }

        uint32_t next = fat_next_cluster(fat, cluster);
        if (fat_is_eof(fat, next))
            break;

        cluster = next;
    }

    return ERR_PTR(FILE_NOT_FOUND);
}

int fat_mkdir(struct inode *dir, const char *name) {
    if (!dir || !name) {
        return INVALID_ARG;
    }

    struct FATFileDescriptor* fd = (struct FATFileDescriptor*)dir->private_data;
    struct FAT* fat = fd->fat;

    if (!(fd->entry.attr & ATTR_DIRECTORY)) {
        return INVALID_ARG;
    }

    return _fat_create(fat, name, fd->firstCluster, ATTR_DIRECTORY);
}

int fat_rmdir(struct inode *dir, const char* name){
    if(!dir){
        return INVALID_ARG;
    }

    struct FATFileDescriptor* dfd = (struct FATFileDescriptor*)dir->private_data;
    struct FAT* fat = dfd->fat;

    if (!(dfd->entry.attr & ATTR_DIRECTORY)) {
        return INVALID_ARG;
    }

    struct inode* tinode = fat_lookup(dir, name);
    if(IS_ERR(tinode)){
        return PTR_ERR(tinode);
    }

    struct FATFileDescriptor* tfd = (struct FATFileDescriptor*)tinode->private_data; 
    int itensCount = _fat_count_entries(fat, tfd->firstCluster);
    inode_destroy(tinode);

    if(IS_STAT_ERR(itensCount)){
        return itensCount;
    }

    if((itensCount - 2) > 0){
        return DIR_NOT_EMPTY;
    }

    return _fat_remove(fat, name, dfd->firstCluster);
}