#include "vfat_fs_internal.h"
#include <def/status.h>
#include <lib/mem.h>

uint8_t fat32_fsinfo_sig_valid(struct FAT32FSInfo* fsInfo){
    return (fsInfo->leadSignature == 0x41615252 &&
            fsInfo->structSignature == 0x61417272 &&
            fsInfo->trailSignature == 0xAA550000);
}

uint32_t fat_next_cluster(struct FAT* fat, uint32_t cluster){
    switch (fat->type) {
        case FAT_TYPE_12: {
            uint32_t offset = (cluster * 3) / 2;
            uint16_t entry;

            if (cluster & 1) {
                entry = ((fat->table.fat12[offset] >> 4) |
                        (fat->table.fat12[offset + 1] << 4)) & 0x0FFF;
            } else {
                entry = (fat->table.fat12[offset] |
                        ((fat->table.fat12[offset + 1] & 0x0F) << 8)) & 0x0FFF;
            }

            return (uint32_t)entry;
        }

        case FAT_TYPE_16: {
            return (uint32_t)(fat->table.fat16[cluster] & 0xFFFF);
        }

        case FAT_TYPE_32: {
            return fat->table.fat32[cluster] & 0x0FFFFFFF;
        }

        default:
            return FAT_INVAL;
    }
}

uint8_t fat_is_eof(struct FAT* fat, uint32_t cluster){
    switch (fat->type) {
        case FAT_TYPE_12: return cluster >= 0x0FF8;
        case FAT_TYPE_16: return cluster >= 0xFFF8;
        case FAT_TYPE_32: return cluster >= 0x0FFFFFF8;
        default: return 0;
    }
}

uint32_t fat_get_eof(struct FAT* fat){
    switch (fat->type) {
        case FAT_TYPE_12: return 0x0FF8;
        case FAT_TYPE_16: return 0xFFF8;
        case FAT_TYPE_32: return 0x0FFFFFF8;
        default: return 0;
    }
}

void fat_add(struct FAT* fat, uint32_t index, uint32_t next){
    switch (fat->type) {
        case FAT_TYPE_12:
            uint32_t offset = (index * 3) / 2;

            if (index & 1) {
                uint16_t current = fat->table.fat12[offset] | (fat->table.fat12[offset + 1] << 8);
                current &= 0x000F;
                current |= (next & 0x0FFF) << 4;
                fat->table.fat12[offset] = current & 0xFF;
                fat->table.fat12[offset + 1] = (current >> 8) & 0xFF;
            } else {
                uint16_t current = fat->table.fat12[offset] | (fat->table.fat12[offset + 1] << 8);
                current &= 0xF000;
                current |= (next & 0x0FFF);
                fat->table.fat12[offset] = current & 0xFF;
                fat->table.fat12[offset + 1] = (current >> 8) & 0xFF;
            }
            break;

        case FAT_TYPE_16:
            fat->table.fat16[index] = (uint16_t)(next & 0xFFFF);
            break;

        case FAT_TYPE_32:
            fat->table.fat32[index] = (fat->table.fat32[index] & 0xF0000000) | (next & 0x0FFFFFFF);
            break;

        default:
            break;
    }
}

uint32_t fat_find_free_cluster(struct FAT* fat){
    uint32_t i;
    uint32_t total = fat->totalClusters;

    if(fat->type == FAT_TYPE_32){
        if(fat->fsInfo.nextFreeCluster >= fat->rootClus && fat->fsInfo.nextFreeCluster < fat->totalClusters){
            i = fat->fsInfo.nextFreeCluster;
            for (; i < total; i++)
            {
                if(fat_next_cluster(fat, i) == FAT_UNUSED){
                    fat->fsInfo.nextFreeCluster = i + 1;
                    return i;
                }
            }
        }
    }

    for (i = 2; i < total; ++i) {
        uint32_t val = fat_next_cluster(fat, i);
        if (fat_is_eof(fat, val) || val == FAT_UNUSED) {
            return i;
        }
    }

    return FAT_INVAL;
}

uint32_t fat_append_cluster(struct FAT* fat, uint32_t from){
    uint32_t clus = fat_find_free_cluster(fat);
    if(clus == FAT_INVAL){
        return clus;
    }

    fat_add(fat, from, clus);
    switch (fat->type) {
        case FAT_TYPE_12: fat_add(fat, clus, 0x0FFF); break;
        case FAT_TYPE_16: fat_add(fat, clus, 0xFFFF); break;
        case FAT_TYPE_32: fat_add(fat, clus, 0x0FFFFFFF); break;
        default: return FAT_INVAL;
    }

    return clus;
}

void fat_free_chain(struct FAT* fat, uint32_t startCluster){
    if (!fat || startCluster < 2 || fat_is_eof(fat, startCluster)){
        return;
    }

    uint32_t cluster = startCluster;
    while (!fat_is_eof(fat, cluster)){
        uint32_t next = fat_next_cluster(fat, cluster);

        fat_add(fat, cluster, FAT_UNUSED);

        if (next < 2 || next == FAT_INVAL) {
            break; // Security
        }

        cluster = next;
    }
}

int64_t fat_get_entry_lba(struct FAT* fat, uint32_t dirCluster, struct FATLegacyEntry* entry){
    if(!entry || dirCluster < 2){
        return INVALID_ARG;
    }

    struct Stream* stream = fat->stream;

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
                return FILE_NOT_FOUND;
            }

            if (buffer.name[0] == 0xE5) {
                continue;
            }

            if(buffer.FstClusLO == entry->FstClusLO && buffer.FstClusHI == entry->FstClusHI){
                return lba + (i * sizeof(buffer));
            }
        }

        cluster = fat_next_cluster(fat, cluster);
    }

    return FILE_NOT_FOUND;
}
