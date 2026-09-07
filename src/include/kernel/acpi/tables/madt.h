#ifndef _ACPI_TABLE_MADT
#define _ACPI_TABLE_MADT

/* Multiple APIC Description Table */

#include <def/compile.h>
#include <def/bits.h>
#include <stdint.h>

#include <kernel/acpi.h>

#define ACPI_MADT_SIGNATURE "APIC"

/* Entry types */
#define MADT_TYPE_LOCAL_APIC             0
#define MADT_TYPE_IO_APIC                1
#define MADT_TYPE_INTERRUPT_OVERRIDE     2
#define MADT_TYPE_NMI_SOURCE             3
#define MADT_TYPE_LOCAL_NMI              4
#define MADT_TYPE_LOCAL_APIC_OVERRIDE    5
#define MADT_TYPE_IO_SAPIC               6
#define MADT_TYPE_LOCAL_SAPIC            7
#define MADT_TYPE_PLATFORM_INTERRUPT     8
#define MADT_TYPE_LOCAL_X2APIC           9
#define MADT_TYPE_LOCAL_X2APIC_NMI       10

/* Flags */
#define MADT_FLAG_PCAT_COMPAT           BIT(0)

#define MADT_LAPIC_FLAG_ENABLED         BIT(0)
#define MADT_LAPIC_FLAG_ONLINE_CAPABLE  BIT(1)

/* Entry Types */
#define MADT_TYPE_LOCAL_APIC           0
#define MADT_TYPE_IO_APIC              1
#define MADT_TYPE_INTERRUPT_OVERRIDE   2
#define MADT_TYPE_NMI_SOURCE           3
#define MADT_TYPE_LOCAL_NMI            4
#define MADT_TYPE_LOCAL_APIC_OVERRIDE  5
#define MADT_TYPE_IO_SAPIC             6
#define MADT_TYPE_LOCAL_SAPIC          7
#define MADT_TYPE_LOCAL_X2APIC         9
#define MADT_TYPE_LOCAL_X2APIC_NMI    10

/* Interrupt flags */
#define MADT_INT_FLAGS_POLARITY_MASK  0x3
#define MADT_INT_FLAGS_TRIGGER_MASK   0xC

#define MADT_INT_POLARITY_DEFAULT     0x0
#define MADT_INT_POLARITY_HIGH        0x1
#define MADT_INT_POLARITY_LOW         0x3

#define MADT_INT_TRIGGER_DEFAULT      0x0
#define MADT_INT_TRIGGER_EDGE         0x4
#define MADT_INT_TRIGGER_LEVEL        0xC

#define MADT_ALL_PROCESSORS 0xFF


struct acpi_madt_entry {
	uint8_t type;
	uint8_t length;
} __packed;

struct acpi_madt {
	struct acpi_sdt_header header;

	uint32_t local_apic_address;
	uint32_t flags;

	struct acpi_madt_entry entries[];
} __packed;

// Type 0: Processor Local APIC
struct acpi_madt_lapic {
	struct acpi_madt_entry entry;
	uint8_t apic_id;      // Local APIC ID
	uint8_t processor_id; // Processor ID
	uint32_t flags;
} __packed;

// Type 1: I/O APIC
struct acpi_madt_ioapic {
	struct acpi_madt_entry entry;
	uint8_t ioapic_id;
	uint8_t reserved;
	uint32_t ioapic_address;
	uint32_t gsi_base;
} __packed;

// Type 2: Interrupt Source Override
struct acpi_madt_int_override {
	struct acpi_madt_entry entry;
	uint8_t bus;            // Bus type (0 = legacy ISA)
	uint8_t source;
	uint32_t gsi;
	uint16_t flags;
} __packed;

// Type 3: I/O APIC Non-maskable interrupt source
struct acpi_madt_nmi_source {
	struct acpi_madt_entry entry;
	uint8_t source;
	uint8_t reserved;
	uint16_t flags;
	uint32_t gsi;
} __packed;

// Type 4: Local APIC Non-maskable interrupts
struct acpi_madt_local_nmi {
	struct acpi_madt_entry entry;
	uint8_t processor_id;
	uint16_t flags;
	uint8_t local_apic_lint; // 0 or 1
} __packed;

// Type 5: Local APIC Address Override
struct acpi_madt_local_apic_address_override {
	struct acpi_madt_entry entry;
	uint16_t reserved;
	uint64_t local_apic_address;
} __packed;

// Type 6: I/O SAPIC
// Type 7: Local SAPIC
// Type 8: Platform Interrupt Source

// Type 9: Processor Local x2APIC
struct acpi_madt_local_x2apic {
	struct acpi_madt_entry entry;
	uint16_t reserved;
	uint32_t x2apic_id;
	uint32_t flags;
	uint32_t acpi_uid;
} __packed;

// Type 10: Local 2xAPIC NMI
// Type 11: GIC CPU Interface (GICC)
// Type 12: GIC Distributor (GICD)
// Type 13: GIC MSI Frame
// Type 14: GIC Redistributor (GICR)
// Type 15: GIC Interrupt Translation Service (ITS)
// Type 16: Multiprocessor Wakeup
// Type 17...127: Reserved
// Type 128...255: OEM defined

#endif
