#ifndef _ACPI_TABLE_FADT
#define _ACPI_TABLE_FADT

/* Fixed ACPI Description Table */

#include <def/compile.h>
#include <def/bits.h>
#include <stdint.h>

#include <kernel/acpi.h>

#define ACPI_FADT_SIGNATURE "FACP"

// Fixed Feature Flags FADT
#define FADT_FLAG_HW_REDUCED_ACPI  BIT(0)
#define FADT_FLAG_WBINVD_NO_FLUSH  BIT(1)
#define FADT_FLAG_PCI_IRQ_ROUTING  BIT(2)
#define FADT_FLAG_WBINVD_FLUSH     BIT(4)
#define FADT_FLAG_S4_SUSPEND       BIT(5)
#define FADT_FLAG_ARB_SUPPORT      BIT(6)
#define FADT_FLAG_RTC_AVAILABLE    BIT(7)

// PM1a_CNT and PM1b_CNT Field Offsets
#define PM1_CNT_OFFSET 0x04
#define PM1_CNT_SIZE   2

// PM1a_CNT Masks
#define PM1_CNT_BUSY    (1 << 15)
#define PM1_CNT_SLP_TYP (0x1FF0)
#define PM1_CNT_SLP_EN  (1 << 13)
#define PM1_CNT_SCI_EN  (1 << 0)
#define PM1_CNT_GBL_EVT_EN (1 << 8)

// PM1 Event Masks
#define PM1_EVT_CNT_VAL (0xFFFF)

// Sleep Types (S1-S5)
#define FADT_SLP_S1 (0x001)
#define FADT_SLP_S2 (0x002)
#define FADT_SLP_S3 (0x003)
#define FADT_SLP_S4 (0x004)
#define FADT_SLP_S5 (0x005)

struct acpi_fadt {
	struct acpi_sdt_header header;

    uint32_t firmware_ctrl;
    uint32_t dsdt;

    uint8_t  reserved;

    uint8_t  preferred_power_management_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t  acpi_enable;
    uint8_t  acpi_disable;
    uint8_t  s4bios_req;
    uint8_t  pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t  pm1_event_length;
    uint8_t  pm1_control_length;
    uint8_t  pm2_control_length;
    uint8_t  pm_timer_length;
    uint8_t  gpe0_length;
    uint8_t  gpe1_length;
    uint8_t  gpe1_base;
    uint8_t  cstate_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t  duty_offset;
    uint8_t  duty_width;
    uint8_t  day_alarm;
    uint8_t  month_alarm;
    uint8_t  century;
    uint16_t boot_architecture_flags;

    uint8_t  reserved2;
    uint32_t flags;

    struct acpi_generic_address reset_register;

    uint8_t  reset_value;
    uint8_t  reserved3[3];
  
    uint64_t x_firmware_control;
    uint64_t x_dsdt;

    struct acpi_generic_address x_pm1a_event_block;
    struct acpi_generic_address x_pm1b_event_block;
    struct acpi_generic_address x_pm1a_control_block;
    struct acpi_generic_address x_pm1b_control_block;
    struct acpi_generic_address x_pm2_control_block;
    struct acpi_generic_address x_pm_timer_block;
    struct acpi_generic_address x_gpe0_block;
    struct acpi_generic_address x_gpe1_block;
} __packed;

#endif
