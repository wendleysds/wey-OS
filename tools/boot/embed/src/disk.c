#include <def/errno.h>
#include <disk.h>
#include <utils.h>
#include <heap.h>
#include <string.h>

static struct GPTPartitionEntry* gpt_parse_partitions(struct disk* disk, uint8_t* partitionCount){
	return ERR_PTR(-ENOSYS);
}

int disk_parse_partitions(struct disk* disk){
	uint8_t sec0[0x200];
	if(!IS_ERR_VALUE(disk->ops->read(disk, 0, sec0, 1))){
		struct MBRPartitionEntry* mbr = (struct MBRPartitionEntry*)(sec0 + 0x1BE);

		uint8_t isMBR = mbr->type != 0xEE;
		uint8_t partitionCount = 0;

		printf("%s Partition detected, parsing...\n", isMBR ? "MBR" : "GPT");

		if(isMBR){
			struct MBRPartitionEntry* partitions = (struct MBRPartitionEntry*)malloc(4 * sizeof(struct MBRPartitionEntry));
			if(!partitions){
				return -ENOMEM;
			}

			memcpy(partitions, mbr, 4 * sizeof(struct MBRPartitionEntry));
			for(partitionCount = 0; partitions[partitionCount].type != 0; partitionCount++);

			disk->mbr_partitions = partitions;
		}else{
			struct GPTPartitionEntry* partitions = gpt_parse_partitions(disk, &partitionCount);
			if(IS_ERR(partitions)){
				return PTR_ERR(partitions);
			}

			disk->gpt_partitions = partitions;
		}

		printf("Parsed %d %s Partition(s)\n", partitionCount, isMBR ? "MBR" : "GPT");

		return SUCCESS;
	}

	return -EIO;
}
