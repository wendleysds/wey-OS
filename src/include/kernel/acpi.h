#ifndef _ACPI_H
#define _ACPI_H

#include <def/compile.h>
#include <io/region.h>
#include <stdbool.h>
#include <stdint.h>

struct rsdp_descriptor_v1 {
	char     signature[8];
	uint8_t  checksum;
	char     oemid[6];
	uint8_t  revision;
	uint32_t rsdt_address;
} __packed;

struct rsdp_descriptor_v2 {
	struct rsdp_descriptor_v1 v1;
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t  extended_checksum;
	uint8_t  reserved[3];
} __packed;

typedef union {
	struct rsdp_descriptor_v1 v1;
	struct rsdp_descriptor_v2 v2;
} rsdp_descriptor_t;

enum acpi_address_space_id {
	ACPI_ADDRESS_SPACE_SYSTEM_MEMORY       = 0x00,
	ACPI_ADDRESS_SPACE_SYSTEM_IO           = 0x01,
	ACPI_ADDRESS_SPACE_PCI_CONFIG          = 0x02,
	ACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER = 0x03,
	ACPI_ADDRESS_SPACE_SMBUS               = 0x04,
	ACPI_ADDRESS_SPACE_SYSTEM_CMOS         = 0x05,
	ACPI_ADDRESS_SPACE_PCI_BAR             = 0x06,
	ACPI_ADDRESS_SPACE_IPMI                = 0x07,
	ACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO  = 0x08,
	ACPI_ADDRESS_SPACE_GENERAL_SERIAL_BUS  = 0x09,
	//0x0B to 0x7F Reserved
	//0x80 to 0xFF OEM Defined 
};

enum acpi_access_size {
	ACPI_ACCESS_SIZE_ANY     = 0,
	ACPI_ACCESS_SIZE_BYTE    = 1,
	ACPI_ACCESS_SIZE_WORD    = 2,
	ACPI_ACCESS_SIZE_DWORD   = 3,
	ACPI_ACCESS_SIZE_QWORD   = 4,
};

struct acpi_generic_address {
	uint8_t address_space_id;
	uint8_t bit_width;
	uint8_t bit_offset;
	uint8_t access_size;
	uint64_t address;
} __packed;

struct acpi_sdt_header {
	char     signature[4];
	uint32_t length;
	uint8_t  revision;
	uint8_t  checksum;
	char     oemid[6];
	uint64_t oem_table_id;
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __packed;

struct acpi_pm_info {
	io_region_t smi_cmd;
    io_region_t pm1a_cnt;
    io_region_t pm1b_cnt;
    io_region_t pm2_cnt;
    io_region_t pm_timer;
    io_region_t gpe0;
    io_region_t gpe1;
    io_region_t reset;

    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_timer_len;
    uint8_t gpe0_len;
    uint8_t gpe1_len;
    uint8_t reset_value;

    uint16_t sci_int;

    uint8_t acpi_enable;
    uint8_t acpi_disable;

	uint16_t SLP_TYPa;
	uint16_t SLP_TYPb;

    bool has_pm1b;
    bool has_pm2;
    bool has_pm_timer;
    bool has_gpe0;
    bool has_gpe1;
    bool has_reset;
	bool has_s5;
};

#endif