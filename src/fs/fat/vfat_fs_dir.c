#include <lib/string.h>
#include <fs/stat.h>
#include <def/errno.h>

#include "vfat_fs_internal.h"

static inline int _fat_count_entries(struct FAT* fat, uint32_t dirCluster){
    if(dirCluster < 2){
        return -EINVAL;
    }

    struct Stream* stream = stream_new(fat->bdev);
	if(!stream){
		return -ENOMEM;
	}

    int count = 0;
    uint32_t cluster = dirCluster;
    while (!fat_is_eof(fat, dirCluster)) {
        uint32_t lba = fat_cluster_to_lba(fat, cluster);
        stream_seek_lba(stream, lba, SEEK_SET);

        for (uint32_t i = 0; i < (fat->clusterSize / sizeof(struct FATLegacyEntry)); i++) {
            struct FATLegacyEntry buffer;
            if (stream_read(stream, &buffer, sizeof(buffer)) < 0) {
				stream_dispose(stream);
                return -EIO;
            }

            if (buffer.name[0] == 0x0) {
				stream_dispose(stream);
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

	stream_dispose(stream);

    return count;
}

struct inode* fat_lookup(struct inode *dir, const char *name){
    if(!dir || !name){
        return ERR_PTR(-EINVAL);
    }

    struct FATFileDescriptor* dirfd = (struct FATFileDescriptor*)dir->private_data;

    if(!(dirfd->entry.attr & ATTR_DIRECTORY)){
        return ERR_PTR(-EINVAL);
    }

    struct FAT* fat = dirfd->fat;
    struct Stream* stream = stream_new(fat->bdev);
	if(!stream){
		return ERR_PTR(-ENOMEM);
	}

    uint32_t cluster = dirfd->firstCluster; 

    struct FATLegacyEntry entry;
    memset(&entry, 0x0, sizeof(entry));

    char fname[12];
    memset(fname, 0x0, sizeof(fname));
    fat_name_generate_short(name, fname);

    while (1) {
        uint32_t lba = fat_cluster_to_lba(fat, cluster);
        stream_seek_lba(stream, lba, SEEK_SET);

        for (uint32_t i = 0; i < (fat->clusterSize / sizeof(entry)); ++i) {
            if (stream_read(stream, &entry, sizeof(entry)) < 0) {
				stream_dispose(stream);
                return ERR_PTR(-EIO);
            }

            if (entry.name[0] == 0x0) {
				stream_dispose(stream);
                return ERR_PTR(-ENOENT);
            }

            if (entry.name[0] == 0xE5) {
                continue;
            }

            if (memcmp((char *)entry.name, fname, 11) != 0) {
                continue;
            }

			struct inode* inode = inode_new(dir->i_sb);
			if(!inode){
				stream_dispose(stream);
				return ERR_PTR(-ENOMEM);
			}

            struct FATFileDescriptor* newfd = (struct FATFileDescriptor*)kmalloc(sizeof(struct FATFileDescriptor));
            if(!newfd){
                inode_destroy(inode);
				stream_dispose(stream);
                return ERR_PTR(-ENOMEM);
            }

            newfd->fat = fat;
            newfd->firstCluster = ((entry.fstClusHI << 16) | entry.fstClusLO);
            newfd->currentCluster = newfd->firstCluster;
            newfd->parentDirCluster = cluster;
            newfd->entry = entry;

			inode->ino = newfd->firstCluster;
            inode->mode = (entry.attr & ATTR_DIRECTORY) ? S_IFDIR : S_IFREG;
            inode->size = entry.fileSize;
            inode->private_data = (void*)newfd;
			atomic_set(&inode->refcount, 1);

            inode->i_op = &vfat_fs_iop;
            inode->i_fop = &vfat_fs_fop;

			stream_dispose(stream);
            return inode;
        }

        uint32_t next = fat_next_cluster(fat, cluster);
        if (fat_is_eof(fat, next))
            break;

        cluster = next;
    }

	stream_dispose(stream);
    return ERR_PTR(-ENOENT);
}

int fat_mkdir(struct inode *dir, const char *name) {
    if (!dir || !name) {
        return -EINVAL;
    }

    struct FATFileDescriptor* fd = (struct FATFileDescriptor*)dir->private_data;
    struct FAT* fat = fd->fat;

    if (!(fd->entry.attr & ATTR_DIRECTORY)) {
        return -EINVAL;
    }

    return _fat_create(fat, name, fd->firstCluster, ATTR_DIRECTORY);
}

int fat_rmdir(struct inode *dir, const char* name){
    if(!dir){
        return -EINVAL;
    }

    struct FATFileDescriptor* dfd = (struct FATFileDescriptor*)dir->private_data;
    struct FAT* fat = dfd->fat;

    if (!(dfd->entry.attr & ATTR_DIRECTORY)) {
        return -EINVAL;
    }

    struct inode* tinode = fat_lookup(dir, name);
    if(IS_ERR(tinode)){
        return PTR_ERR(tinode);
    }

    struct FATFileDescriptor* tfd = (struct FATFileDescriptor*)tinode->private_data; 
    int itensCount = _fat_count_entries(fat, tfd->firstCluster);
    inode_put(tinode);

    if(IS_ERR_VALUE(itensCount)){
        return itensCount;
    }

    if((itensCount - 2) > 0){
        return -ENOTEMPTY;
    }

    return _fat_remove(fat, name, dfd->firstCluster);
}