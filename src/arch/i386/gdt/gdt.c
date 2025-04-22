#include <arch/i386/gdt.h>
#include <core/kernel.h>
#include <stdint.h>

void encode_gdt_entry(uint8_t* target, struct GDT_Structured source){
    if ((source.limit > 65536) && ((source.limit & 0xFFF) != 0xFFF)){
        panic("encode_gdt_entry: Invalid argument\n");
    }

		if (source.limit > 0xFFFFF){
			panic("GDT cannot encode limits larger than 0xFFFFF");
		}

    // Encodes the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] |= (source.limit >> 16) & 0x0F;

    // Encode the base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;

    // Set the type
    target[5] = source.type;

		// Encode the flags
		target[6] |= (source.flags << 4);
}

void gdt_structured_to_gdt(struct GDT *gdt, struct GDT_Structured *structured_gdt, int total_entires){
    for(int i = 0; i < total_entires; i++){
        encode_gdt_entry((uint8_t*)&gdt[i], structured_gdt[i]);
    }
}
