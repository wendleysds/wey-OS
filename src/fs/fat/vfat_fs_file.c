#include <lib/string.h>
#include <def/errno.h>

#include "vfat_fs_internal.h"

static inline uint8_t _lfn_chksum(const uint8_t* shortName){
    uint8_t sum = 0;

    for (uint8_t i = 0; i < 11; i++){
        // NOTE: The operation is an unsigned char rotate right
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + shortName[i];
    }

    return sum;
}

static int _fat_lfn_read(struct Stream* stream, const uint8_t* shortName, size_t entryLBA, char* lfnBuffer, size_t maxSize){
    return -ENOSYS;
}

static int _fat_lfn_write(struct Stream* stream, const uint8_t* shortName, size_t entryLBA, char* lfnBuffer, size_t maxSize){
    return -ENOSYS;
}

int _fat_create(struct FAT* fat, const char* filename, uint32_t dirCluster, int attr){
    if(!fat || !filename){
        return -EINVAL;
    }

    if(strlen(filename) > 11){
        return -ENOSYS; // lfn not supported wet
    }

    struct Stream* stream = stream_new(fat->bdev);
	if(!stream){
		return -ENOMEM;
	}

    struct FATLegacyEntry buffer;
    memset(&buffer, 0x0, sizeof(buffer));

    char name[12] = {0};
    fat_name_generate_short(filename, name);

    int collisions = 0;

    uint32_t cluster = dirCluster;
    while(1) {
        uint32_t lba = fat_cluster_to_lba(fat, cluster);
        stream_seek_lba(stream, lba, SEEK_SET);

        for(uint32_t i = 0; i < (fat->clusterSize / sizeof(struct FATLegacyEntry)); i++) {
            if(stream_read(stream, &buffer, sizeof(buffer)) < 0) {
				stream_dispose(stream);
                return -EIO;
            }

            if((buffer.attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME){
                continue;
            }

            if(memcmp((char*)buffer.name, name, 11) == 0){
                if (strlen(filename) <= 11){
                    // TODO: Check if hava an LFN above
                    // if not
					stream_dispose(stream);
                    return -EEXIST;
                }

                // TODO: Parse LFN and compare with filename
                // if equals
                //   return -EEXIST
                // else
                    fat_name_append_tilde(name, ++collisions);
                
                continue;
            }

            if(buffer.name[0] != 0x0 && buffer.name[0] != 0xE5){
                continue;
            }

            uint32_t _cluster = fat_find_free_cluster(fat);
            switch (fat->type) {
                case FAT_TYPE_12: fat_add(fat, _cluster, 0x0FFF); break;
                case FAT_TYPE_16: fat_add(fat, _cluster, 0xFFFF); break;
                case FAT_TYPE_32: fat_add(fat, _cluster, 0x0FFFFFFF); break;
                default:
					stream_dispose(stream);
                    return -EINVAL;
            }

            memset(&buffer, 0x0, sizeof(buffer));
            memcpy(buffer.name, name, 11);
            buffer.attr = attr;

            if(fat->type == FAT_TYPE_32){
                buffer.fstClusHI = _cluster >> 16;
                buffer.fstClusLO = _cluster;
            }else{
                buffer.fstClusHI = 0;
                buffer.fstClusLO = _cluster;
            }

            stream_seek(stream, -sizeof(buffer), SEEK_CUR);
            if(stream_write(stream, &buffer, sizeof(buffer)) < 0) {
				stream_dispose(stream);
                return -EIO;
            }

            if(attr & ATTR_DIRECTORY){
                stream_seek_lba(stream, fat_cluster_to_lba(fat, _cluster), SEEK_SET);

                struct FATLegacyEntry dot_entry;
                memset(&dot_entry, 0x0, sizeof(dot_entry));
                
                memcpy(dot_entry.name, "..         ", 11);
                dot_entry.attr = ATTR_DIRECTORY;

                if(fat->type == FAT_TYPE_32){
                    dot_entry.fstClusHI = (dirCluster >> 16) & 0xFFFF;
                    buffer.fstClusLO = dirCluster & 0xFFFF;
                }else{
                    dot_entry.fstClusHI = 0;
                    dot_entry.fstClusLO = dirCluster & 0xFFFF;
                }

                if(stream_write(stream, &dot_entry, sizeof(dot_entry)) < 0) {
                    _fat_remove(fat, name, dirCluster);
					stream_dispose(stream);
                    return -EIO;
                }
                
                memcpy(dot_entry.name, ".          ", 11);

                if(fat->type == FAT_TYPE_32){
                    dot_entry.fstClusHI = (_cluster >> 16) & 0xFFFF;
                    buffer.fstClusLO = _cluster & 0xFFFF;
                }else{
                    dot_entry.fstClusHI = 0;
                    dot_entry.fstClusLO = _cluster & 0xFFFF;
                }

                if(stream_write(stream, &dot_entry, sizeof(dot_entry)) < 0) {
                    _fat_remove(fat, name, dirCluster);
					stream_dispose(stream);
                    return -EIO;
                }
            }

			stream_dispose(stream);
            return fat_update(fat);
        }

        uint32_t previus = cluster;
        cluster = fat_next_cluster(fat, cluster);

        extern void panic(const char* fmt, ...);
        panic("End of %d!", dirCluster);

        // Reserve a new cluster for the directory if necessary.
        if(fat_is_eof(fat, cluster)){
            cluster = fat_append_cluster(fat, previus);
            if(cluster == FAT_INVAL){
				stream_dispose(stream);
                return -ERANGE; // FAT Full
            }
        }
    }
}

int _fat_remove(struct FAT* fat, const char* filename, uint32_t dirCluster){
	if(!fat || !filename){
		return -EINVAL;
	}

    struct Stream* stream = stream_new(fat->bdev);
	if(!stream){
		return -ENOMEM;
	}

    uint32_t cluster = dirCluster; 

    struct FATLegacyEntry entry;
    int8_t status = SUCCESS;

    char name[12];
    fat_name_generate_short(filename, name);

    while (1) {
        uint32_t lba = fat_cluster_to_lba(fat, cluster);
        stream_seek_lba(stream, lba, SEEK_SET);

        for (uint32_t i = 0; i < (fat->clusterSize / sizeof(entry)); ++i) {
            if ((status = stream_read(stream, &entry, sizeof(entry))) < 0)
				stream_dispose(stream);
                return status;

            if (entry.name[0] == 0x0) {
				stream_dispose(stream);
				return -ENOENT;
			}

			if (entry.name[0] == 0xE5) {
				continue;
			}

			if (memcmp((char *)entry.name, name, 11) != 0) {
				continue;
			}

            uint32_t fileCluster = ((entry.fstClusHI << 16) | entry.fstClusLO);
            uint8_t del = 0xE5;

            stream_seek(stream, -sizeof(entry), SEEK_CUR);
            stream_write(stream, &del, 1);
            fat_free_chain(fat, fileCluster);

			stream_dispose(stream);
            return fat_update(fat);
        }

        uint32_t next = fat_next_cluster(fat, cluster);
        if (fat_is_eof(fat, next))
            break;

        cluster = next;
    }

	stream_dispose(stream);
    return -ENOENT;
}

int fat_create(struct inode *dir, const char *name, uint16_t mode){
    if(!dir || !name){
        return -EINVAL;
    }

    struct FATFileDescriptor* fd = (struct FATFileDescriptor*)dir->private_data;

    return _fat_create(fd->fat, name, fd->firstCluster, ATTR_ARCHIVE);
}

int fat_unlink(struct inode *dir, const char *name){
    if(!dir || !name){
        return -EINVAL;
    }

    struct FATFileDescriptor* fd = (struct FATFileDescriptor*)dir->private_data;

    return _fat_remove(fd->fat, name, fd->firstCluster);
}

int fat_getattr(struct inode *dir, const char *name, struct stat* restrict statbuf){
    if(!dir || !name || !statbuf){
        return -EINVAL;
    }

    struct inode* ino = fat_lookup(dir, name);
    if(IS_ERR(ino)){
        return PTR_ERR(ino);
    }

    struct FATFileDescriptor* fd = (struct FATFileDescriptor*)ino->private_data;

    statbuf->mode = ino->mode;
    statbuf->size = ino->size;
    statbuf->uid = 0;
    statbuf->attr = fd->entry.attr;
    
    statbuf->atime = fd->entry.lstAccDate;
    statbuf->mtime = fd->entry.wrtDate;
    statbuf->ctime = fd->entry.crtDate;

    return SUCCESS;
}

int fat_setarrt(struct inode *dir, const char *name, uint16_t attr){
    if(!dir || !name){
        return -EINVAL;
    }

    return -ENOSYS;
}
