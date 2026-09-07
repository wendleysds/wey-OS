#include <kernel/printk.h>
#include <kernel/acpi.h>

#include <lib/assert.h>
#include <lib/string.h>
#include <def/errno.h>

#include <asm/page.h>

#include "../internal.h"

extern struct acpi_pm_info acpi_pm;

static void acpi_parse_smi_cmd(struct acpi_fadt *fadt) {
	acpi_pm.smi_cmd.type = IO_TYPE_PIO;
	acpi_pm.smi_cmd.pio_base = fadt->smi_command_port;

	acpi_pm.acpi_enable = fadt->acpi_enable;
	acpi_pm.acpi_disable = fadt->acpi_disable;
}

static void acpi_parse_pm1a_cnt_blk(struct acpi_fadt *fadt)
{
	acpi_pm.pm1a_cnt.type = IO_TYPE_PIO;
	acpi_pm.pm1a_cnt.pio_base = fadt->pm1a_control_block;

	if (acpi_pm.pm1a_cnt.pio_base == 0) {
		return;
	}
}

static void acpi_parse_pm1b_cnt_blk(struct acpi_fadt *fadt)
{
	acpi_pm.pm1b_cnt.type = IO_TYPE_PIO;
	acpi_pm.pm1b_cnt.pio_base = fadt->pm1b_control_block;

	if (acpi_pm.pm1b_cnt.pio_base == 0) {
		return;
	}

	acpi_pm.has_pm1b = 1;
}

static void acpi_parse_pm2_cnt_blk(struct acpi_fadt *fadt)
{
	acpi_pm.pm2_cnt.type = IO_TYPE_PIO;
	acpi_pm.pm2_cnt.pio_base = fadt->pm2_control_block;
	acpi_pm.pm2_cnt_len = fadt->pm2_control_length;

	if (acpi_pm.pm2_cnt.pio_base == 0) {
		return;
	}

	if (acpi_pm.pm2_cnt_len == 0) {
		acpi_pm.pm2_cnt.pio_base = 0;
		return;
	}

	acpi_pm.has_pm2 = 1;
}

static void acpi_parse_pm_timer_blk(struct acpi_fadt *fadt)
{
	acpi_pm.pm_timer.type = IO_TYPE_PIO;
	acpi_pm.pm_timer.pio_base = fadt->pm_timer_block;
	acpi_pm.pm_timer_len = fadt->pm_timer_length;

	if (acpi_pm.pm_timer.pio_base == 0) {
		return;
	}

	if (acpi_pm.pm_timer_len != 4) {
		acpi_pm.pm_timer.pio_base = 0;
		return;
	}

	acpi_pm.has_pm_timer = 1;
}

static void acpi_parse_gpe0_blk(struct acpi_fadt *fadt)
{
	acpi_pm.gpe0.type = IO_TYPE_PIO;
	acpi_pm.gpe0.pio_base = fadt->gpe0_block;
	acpi_pm.gpe0_len = fadt->gpe0_length;

	if (acpi_pm.gpe0.pio_base == 0 || acpi_pm.gpe0_len == 0) {
		return;
	}

	acpi_pm.has_gpe0 = 1;
}

static void acpi_parse_gpe1_blk(struct acpi_fadt *fadt)
{
	acpi_pm.gpe1.type = IO_TYPE_PIO;
	acpi_pm.gpe1.pio_base = fadt->gpe1_block;
	acpi_pm.gpe1_len = fadt->gpe1_length;

	if (acpi_pm.gpe1.pio_base == 0 || acpi_pm.gpe1_len == 0) {
		return;
	}

	acpi_pm.has_gpe1 = 1;
}

static void acpi_parse_reset(struct acpi_fadt *fadt)
{
	if (acpi_gas_to_io_region(&fadt->reset_register, &acpi_pm.reset) != 0) {
		return;
	}

	acpi_pm.has_reset = 1;	
	acpi_pm.reset_value = fadt->reset_value;
}

static inline u8 acpi_parse_aml_field(u8 **ptr) {
	if (**ptr == 0x0A) (*ptr)++;
	return *((*ptr)++);
}

static uint8_t *acpi_find_s5(uint8_t *aml, size_t length) {
	static const uint8_t root_s5[] = {0x08, '\\', '_', 'S', '5', '_'};
	static const uint8_t local_s5[] = {0x08, '_', 'S', '5', '_'};

	for (size_t i = 0; i < length; i++) {

		if (i + sizeof(root_s5) <= length &&
			memcmp(&aml[i], root_s5, sizeof(root_s5)) == 0) {

			return &aml[i];
		}

		if (i + sizeof(local_s5) <= length &&
			memcmp(&aml[i], local_s5, sizeof(local_s5)) == 0) {

			return &aml[i];
		}
	}

	return NULL;
}

static void acpi_parse_s5(struct acpi_fadt *fadt) {
	uintptr_t dsdt_addr = fadt->dsdt ? fadt->dsdt : (uintptr_t)fadt->x_dsdt;
	if (!dsdt_addr) return;

    struct acpi_sdt_header *dsdt = (struct acpi_sdt_header *) __va(dsdt_addr);

	if (memcmp(dsdt->signature, "DSDT", 4) != 0) {
		return;
	}

	if (!acpi_checksum_ok(dsdt, dsdt->length)) {
		return;
	}

    u8 *curr = (u8 *) dsdt + sizeof(struct acpi_sdt_header);
    size_t remaining = dsdt->length - sizeof(struct acpi_sdt_header);

    u8 *s5_addr = NULL;
    while (remaining > 3) {
        if (memcmp(curr, "_S5_", 4) == 0) {
            s5_addr = curr;
            break;
        }
        curr++;
        remaining--;
    }

    if (!s5_addr) {
        return;
    }

    int has_prefix = (s5_addr >= (u8 *)dsdt + 2 && *(s5_addr - 1) == '\\' && *(s5_addr - 2) == 0x08);
    int has_name_op = (s5_addr >= (u8 *)dsdt + 1 && *(s5_addr - 1) == 0x08);

    if (!(has_prefix || has_name_op) || s5_addr[4] != 0x12) {
        return;
    }

    s5_addr += 5;
    
    int pkg_len_bytes = ((*s5_addr & 0xC0) >> 6) + 2;
    s5_addr += pkg_len_bytes;

    acpi_pm.SLP_TYPa = (u16)acpi_parse_aml_field(&s5_addr) << 10;
    acpi_pm.SLP_TYPb = (u16)acpi_parse_aml_field(&s5_addr) << 10;

	acpi_pm.has_s5 = true;
}

int acpi_reboot(void) {
	if (!acpi_pm.has_reset || acpi_pm.reset_value == 0) {
		return -ENODEV;
	}

	io_write8(&acpi_pm.reset, 0, acpi_pm.reset_value);
	unreachable();
}

int acpi_shutdown(void) {
	if (!acpi_pm.has_s5) return -ENODEV;

	io_write16(&acpi_pm.pm1a_cnt, 0, acpi_pm.SLP_TYPa | PM1_CNT_SLP_EN);
	if (acpi_pm.has_pm1b) {
		io_write16(&acpi_pm.pm1b_cnt, 0, acpi_pm.SLP_TYPb | PM1_CNT_SLP_EN);
	}

	return -EIO;
}

void acpi_parse_fadt(struct acpi_fadt *fadt)
{
	if (!fadt)
		return;

	acpi_parse_smi_cmd(fadt);

	acpi_parse_pm1a_cnt_blk(fadt);
	acpi_parse_pm1b_cnt_blk(fadt);

	acpi_parse_pm2_cnt_blk(fadt);
	acpi_parse_pm_timer_blk(fadt);

	acpi_parse_gpe0_blk(fadt);
	acpi_parse_gpe1_blk(fadt);

	acpi_parse_reset(fadt);
	acpi_parse_s5(fadt);

	acpi_pm.fadt = fadt;
}
