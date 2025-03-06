#ifndef _GDT_H
#define _GDT_H
#include <stdint.h>

struct GDT{
    uint16_t segment;
    uint16_t base_first;
    uint8_t base;
    uint8_t access;
    uint8_t high_flags;
    uint8_t base_24_31_bits;
}__attribute__((packed));

struct GDT_Structured{
    uint32_t base;
    uint32_t limit;
    uint8_t type;
};

void gdt_load(struct GDT* gdt, int size);
void gdt_structured_to_gdt(struct GDT* gdt, struct GDT_structured* structured_gdt, int total_entires);

#endif