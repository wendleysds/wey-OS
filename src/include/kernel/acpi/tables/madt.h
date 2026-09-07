#ifndef _ACPI_TABLE_MADT
#define _ACPI_TABLE_MADT

/* Multiple APIC Description Table */

#include <def/compile.h>
#include <def/bits.h>
#include <stdint.h>

#include <kernel/acpi.h>

#define ACPI_MADT_SIGNATURE "APIC"

// MADT Flags
#define MADT_FLAG_PC_AT_COMPAT   BIT(0)
#define MADT_FLAG_LEGACY_PIC     BIT(1)
#define MADT_FLAG_FLOATING_PIC   BIT(2)
#define MADT_FLAG_ELCR_BASE_ADDR BIT(3)
#define MADT_FLAG_EXT_PIC_BASE   BIT(4)
#define MADT_FLAG_64BIT_APIC     BIT(12)
#define MADT_FLAG_82489DX_APIC   BIT(13)
#define MADT_FLAG_PC_AT_COMPAT2  BIT(14)
#define MADT_FLAG_CMOS_RTC_NMI   BIT(15)
#define MADT_FLAG_APIC_OVERRIDE  BIT(16)
#define MADT_FLAG_PC_AT_8254_RTC BIT(17)
#define MADT_FLAG_CMOS_RTC_PIC   BIT(18)

// Tipos de Entrada MADT
#define MADT_TYPE_LOCAL_APIC       (0)
#define MADT_TYPE_IO_APIC          (1)
#define MADT_TYPE_INT_SRC_OVRERR   (2)
#define MADT_TYPE_LOCAL_NMI        (4)
#define MADT_TYPE_LOCAL_APIC_NO_CPU (5)
#define MADT_TYPE_IO_SAPIC         (6)
#define MADT_TYPE_LOCAL_X2APIC     (8)
#define MADT_TYPE_LOCAL_X2XAPIC    (9)
#define MADT_TYPE_THERMAL_APIC     (12)
#define MADT_TYPE_THERMAL_X2APIC   (13)

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
	uint32_t apic_id;      // Local APIC ID
	uint32_t processor_id; // Processor ID
	uint32_t flags;
	uint8_t reserved[4];
} __packed;

// Type 1: I/O APIC
struct acpi_madt_ioapic {
	struct acpi_madt_entry entry;
	uint32_t ioapic_id;      // I/O APIC ID
	uint8_t reserved1;
	uint8_t reserved2;
	uint32_t gsi_base;       // GSI base (Global System Interrupt base)
	uint64_t ioapic_addr;
} __packed;

// Type 2: Interrupt Source Override
struct acpi_madt_intsrc_override {
	struct acpi_madt_entry entry;
	uint8_t bus;            // Bus type (0 = legacy ISA)
	uint8_t source;
	uint32_t gsi;
	uint16_t flags;
} __packed;

// Type 3: I/O APIC Non-maskable interrupt source
// Type 4: Local APIC Non-maskable interrupts
// Type 5: Local APIC Address Override
// Type 6: I/O SAPIC
// Type 7: Local SAPIC
// Type 8: Platform Interrupt Source
// Type 9: Processor Local x2APIC
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
