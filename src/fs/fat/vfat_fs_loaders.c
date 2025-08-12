#include "vfat_fs_internal.h"
#include <def/err.h>
#include <io/stream.h>
#include <mmu.h>
#include <drivers/terminal.h>

static int fat12_load(struct FAT* fat, struct Stream* stream, const uint8_t* sector0Buffer){
    memcpy(
        &fat->headers.boot, 
        sector0Buffer, 
        sizeof(struct FATHeader)
    );

    memcpy(
        &fat->headers.extended.fat12, 
        sector0Buffer + sizeof(struct FATHeader),
        sizeof(struct FATHeaderExtended)
    );

    int8_t status;
    const struct FATHeader* boot = &fat->headers.boot;

    uint32_t fatStartSector = boot->rsvdSecCnt;
    uint32_t fatSize = boot->FATSz16;
    uint32_t fatBytes = fatSize * boot->bytesPerSec;
    uint32_t fatClusterSize = boot->bytesPerSec * boot->secPerClus;
    uint32_t rootDirSectors = ((boot->rootEntCnt * 32) + (boot->bytesPerSec - 1)) / boot->bytesPerSec;
    uint32_t firstDataSector = fatStartSector + (boot->numFATs * fatSize) + rootDirSectors;

    uint32_t fatTotalClusters = (boot->totSec16 ? boot->totSec16 : boot->totSec32) - firstDataSector;
    fatTotalClusters /= boot->secPerClus;

    if (fatTotalClusters < 1 || fatTotalClusters >= 4085){
        return INVALID_FS;
    }

    uint8_t *table = (uint8_t *)kmalloc(fatBytes);
    if (!table){
        return NO_MEMORY;
    }

    stream_seek(stream, _SEC(fatStartSector), SEEK_SET);
    if ((status = stream_read(stream, table, fatBytes)) != SUCCESS) {
        kfree(table);
        return status;
    }

    fat->table.fat12 = table;
    fat->firstDataSector = firstDataSector;
    fat->totalClusters = fatTotalClusters;
    fat->rootSector = firstDataSector - rootDirSectors;
    fat->clusterSize = fatClusterSize;

    return SUCCESS;
}

static int fat16_load(struct FAT* fat, struct Stream* stream, const uint8_t* sector0Buffer){
    memcpy(
        &fat->headers.boot, 
        sector0Buffer, 
        sizeof(struct FATHeader)
    );

    memcpy(
        &fat->headers.extended.fat16, 
        sector0Buffer + sizeof(struct FATHeader),
        sizeof(struct FATHeaderExtended)
    );

    int8_t status;
    const struct FATHeader* boot = &fat->headers.boot;

    uint32_t fatStartSector = boot->rsvdSecCnt;
    uint32_t fatSize = boot->FATSz16;
    uint32_t fatBytes = fatSize * boot->bytesPerSec;
    uint32_t rootDirSectors = ((boot->rootEntCnt * 32) + (boot->bytesPerSec - 1)) / boot->bytesPerSec;
    uint32_t firstDataSector = fatStartSector + (boot->numFATs * fatSize) + rootDirSectors;
    uint32_t fatClusterSize = boot->bytesPerSec * boot->secPerClus;

    uint32_t fatTotalClusters = (boot->totSec16 ? boot->totSec16 : boot->totSec32) - firstDataSector;
    fatTotalClusters /= boot->secPerClus;

    if (fat->totalClusters < 4085 || fat->totalClusters >= 65525){
        return INVALID_FS;
    }

    uint16_t *table = (uint16_t *)kmalloc(fatBytes);
    if (!table){
        return NO_MEMORY;
    }

    stream_seek(stream, _SEC(fatStartSector), SEEK_SET);
    if ((status = stream_read(stream, table, fatBytes)) != SUCCESS) {
        kfree(table);
        return status;
    }

    fat->table.fat16 = table;
    fat->firstDataSector = firstDataSector;
    fat->totalClusters = fatTotalClusters;
    fat->rootSector = firstDataSector - rootDirSectors;
    fat->clusterSize = fatClusterSize;

    return SUCCESS;
}

static int fat32_load(struct FAT* fat, struct Stream* stream, const uint8_t* sector0Buffer){
    memcpy(
        &fat->headers.boot, 
        sector0Buffer, 
        sizeof(struct FATHeader)
    );

    memcpy(
        &fat->headers.extended.fat32, 
        sector0Buffer + sizeof(struct FATHeader),
        sizeof(struct FAT32HeaderExtended)
    );

    int8_t status;
    const struct FATHeader* boot = &fat->headers.boot;
    const struct FAT32HeaderExtended* extended = &fat->headers.extended.fat32;

    stream_seek(stream, _SEC(extended->FSInfo), SEEK_SET);
    if((status = stream_read(stream, &fat->fsInfo, sizeof(fat->fsInfo))) != SUCCESS){
        return status;
    }

    uint32_t fatStartSector = boot->rsvdSecCnt;
    uint32_t fatSize = extended->FATSz32;
    uint32_t fatBytes = fatSize * boot->bytesPerSec;
    uint32_t firstDataSector = fatStartSector + (boot->numFATs * fatSize);
    uint32_t fatClusterSize = boot->bytesPerSec * boot->secPerClus;

    uint32_t fatTotalClusters = fatSize * boot->bytesPerSec / 4;

    if (!fat32_fsinfo_sig_valid(&fat->fsInfo)){
        fat->fsInfo.nextFreeCluster = -1;
    }

    uint32_t *table = (uint32_t *)kmalloc(fatBytes);
    if (!table){
        return NO_MEMORY;
    }

    stream_seek(stream, _SEC(fatStartSector), SEEK_SET);
    if((status = stream_read(stream, table, fatBytes)) != SUCCESS){
        kfree(table);
        return status;
    }

    fat->table.fat32 = table;
    fat->firstDataSector = firstDataSector;
    fat->totalClusters = fatTotalClusters;
    fat->rootClus = extended->rootClus;
    fat->clusterSize = fatClusterSize;

    return SUCCESS;
}

_fat_loader_func_t loaders[] = {
    [0] = fat12_load,
    [1] = fat16_load,
    [2] = fat32_load,
};