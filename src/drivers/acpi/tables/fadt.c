#include <kernel/printk.h>
#include <kernel/acpi.h>
#include <def/errno.h>

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

	acpi_pm.fadt = fadt;
}