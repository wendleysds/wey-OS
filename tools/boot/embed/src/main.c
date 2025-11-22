#include <utils.h>
#include <platform.h>
#include <def/err.h>

void main(){
	int status = platform_init();
	if (IS_STAT_ERR(status)){
		printf("Failer to init platform! %d\n", status);
		platform_die();
	}

	struct e820_entry entries[32];
	uint32_t counter = 0;
	status = platform_get_memory_map(entries, sizeof(entries), &counter);

	if (IS_STAT_ERR(status)){
		printf("failed to get memory map\n");
	}else{
		printf("Memory Map Entries: %u\n", counter);
		for (uint32_t i = 0; i < counter; i++){
			uint64_t end = entries[i].base_addr + entries[i].length;

			printf("[0x%x%x - 0x%x%x] %d ",
				(uint32_t)(entries[i].base_addr >> 32),
				(uint32_t)(entries[i].base_addr & 0xFFFFFFFF),
				(uint32_t)(end >> 32),
				(uint32_t)(end & 0xFFFFFFFF),
				entries[i].acpi_attrs
			);

			switch (entries[i].type)
			{
				case E820_TYPE_RAM:
					printf("usable");
					break;
				case E820_TYPE_RESERVED:
					printf("reserved");
					break;
				case E820_TYPE_SOFT_RESERVED:
					printf("soft reserved");
					break;
				case E820_TYPE_ACPI:
					printf("ACPI data");
					break;
				case E820_TYPE_NVS:
					printf("ACPI NVS");
					break;
				case E820_TYPE_UNUSABLE:
					printf("unusable");
					break;
				case E820_TYPE_PMEM:
				case E820_TYPE_PRAM:
					printf("persistent (type %u)", entries[i].type);
					break;
				default:
					printf("type %u", entries[i].type);
					break;
			}

			printf("\n");
		}
	}

	__hlt;
}
