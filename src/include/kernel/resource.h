#ifndef _KERNEL_RESOURCE_H
#define _KERNEL_RESOURCE_H

#include <def/bits.h>
#include <stdint.h>

typedef enum {
    RESOURCE_TYPE_MEMORY = 0,
    RESOURCE_TYPE_IO     = 1,
    RESOURCE_TYPE_IRQ    = 2,
    RESOURCE_TYPE_DMA    = 3,
    RESOURCE_TYPE_BUS    = 4,
} resource_type_t;

typedef enum {
    RESOURCE_FLAG_READABLE    = BIT(0),
    RESOURCE_FLAG_WRITABLE    = BIT(1),
    RESOURCE_FLAG_EXECUTABLE  = BIT(2),
    RESOURCE_FLAG_MMIO        = BIT(3),  /* RESOURCE_TYPE_MEMORY: MMIO mapping */
    RESOURCE_FLAG_IO          = BIT(4),  /* RESOURCE_TYPE_IO:     port-mapped I/O */
    RESOURCE_FLAG_PREFETCH    = BIT(5),  /* RESOURCE_TYPE_MEMORY: prefetchable     */
} resource_flags_t;

struct resource {
    const char       *name;
    resource_type_t   type;
    resource_flags_t  flags;

    union {
        struct {
            uint64_t base;
            uint64_t size;
        } range; // MEMORY, BUS and DMA
        struct {
            uint16_t base;
            uint16_t size;
        } io;
        uint32_t irq;
    };

    struct resource *parent;
    struct resource *sibling;
    struct resource *child;
};

#endif