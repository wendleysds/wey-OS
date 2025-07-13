#include <fs/fat/fat32.h>
#include <fs/fat/fatdefs.h>
#include <io/stream.h>
#include <def/status.h>
#include <def/config.h>
#include <lib/string.h>
#include <lib/utils.h>
#include <lib/mem.h>
#include <memory/kheap.h>
#include "fat32_internal.h"

static uint32_t _find_free_cluster(struct FAT* fat){
	if(!fat || !fat->table){
		return 0xFFFFFFFF;
	}

	uint32_t cluster;
	if(fat->fsInfo.nextFreeCluster >= fat->rootClus && fat->fsInfo.nextFreeCluster < fat->totalClusters){
		cluster = fat->fsInfo.nextFreeCluster;
		for (; cluster < fat->totalClusters; cluster++)
		{
			if(_fat32_next_cluster(fat, cluster) == FAT_UNUSED){
				return cluster;
			}
		}
	}

	cluster = fat->rootClus;
	for (; cluster < fat->totalClusters; cluster++)
	{
		if(_fat32_next_cluster(fat, cluster) == FAT_UNUSED){
			return cluster;
		}
	}

	return 0xFFFFFFFF;
}

uint8_t _fat32_fsinfo_sig_valid(struct FAT32FSInfo* fsInfo) {
	return (fsInfo->leadSignature == 0x41615252 &&
        	fsInfo->structSignature == 0x61417272 &&
        	fsInfo->trailSignature == 0xAA550000);
}

uint32_t _fat32_cluster_to_lba(struct FAT* fat, uint32_t cluster){
	return fat->firstDataSector + ((cluster - 2) * fat->headers.boot.secPerClus);
}

uint32_t _fat32_next_cluster(struct FAT* fat, uint32_t current){
	return fat->table[current] & 0x0FFFFFFF; // Mask to get the cluster number
}

uint32_t _fat32_reserve_next_cluster(struct FAT* fat){
	uint32_t cluster = _find_free_cluster(fat);
	if(cluster == 0xFFFFFFFF){
		return cluster;
	}

	fat->fsInfo.nextFreeCluster = cluster + 1;
	if (fat->fsInfo.freeClusterCount != 0xFFFFFFFF) {
		fat->fsInfo.freeClusterCount--;
	}

	return cluster;
}
