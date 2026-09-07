#include <kernel/printk.h>

#include "../internal.h"

struct acpi_pm_info {
	uint32_t smi_cmd;
	uint8_t  acpi_enable;
	uint8_t  acpi_disable;

	uint32_t pm1a_cnt;
	uint32_t pm1b_cnt;
	uint8_t  pm1_cnt_len;

	uint32_t pm2_cnt;
	uint8_t  pm2_cnt_len;

	uint32_t pm_timer;
	uint8_t  pm_timer_len;

	uint32_t gpe0;
	uint8_t  gpe0_len;

	uint32_t gpe1;
	uint8_t  gpe1_len;

	uint8_t  has_pm2;
	uint8_t  has_pm_timer;
	uint8_t  has_gpe0;
	uint8_t  has_gpe1;
	uint8_t  has_reset;
};

static struct acpi_pm_info acpi_pm;

static void acpi_parse_smi_cmd(struct acpi_fadt *fadt)
{
	acpi_pm.smi_cmd = fadt->smi_command_port;
	acpi_pm.acpi_enable = fadt->acpi_enable;
	acpi_pm.acpi_disable = fadt->acpi_disable;

	printk("ACPI: SMI_CMD = %08x\n", acpi_pm.smi_cmd);
	printk("ACPI: ACPI_ENABLE = %02x\n", acpi_pm.acpi_enable);
	printk("ACPI: ACPI_DISABLE = %02x\n", acpi_pm.acpi_disable);
}

static void acpi_parse_pm1a_cnt_blk(struct acpi_fadt *fadt)
{
	acpi_pm.pm1a_cnt = fadt->pm1a_control_block;

	if (acpi_pm.pm1a_cnt == 0) {
		printk("ACPI: PM1a_CNT_BLK not present\n");
		return;
	}

	printk("ACPI: PM1a_CNT_BLK = %08x\n",
		acpi_pm.pm1a_cnt);
}

static void acpi_parse_pm1b_cnt_blk(struct acpi_fadt *fadt)
{
	acpi_pm.pm1b_cnt = fadt->pm1b_control_block;

	if (acpi_pm.pm1b_cnt == 0) {
		printk("ACPI: PM1b_CNT_BLK not present\n");
		return;
	}

	printk("ACPI: PM1b_CNT_BLK = %08x\n",
		acpi_pm.pm1b_cnt);
}

static void acpi_parse_pm2_cnt_blk(struct acpi_fadt *fadt)
{
	acpi_pm.pm2_cnt = fadt->pm2_control_block;
	acpi_pm.pm2_cnt_len = fadt->pm2_control_length;

	if (acpi_pm.pm2_cnt == 0) {
		printk("ACPI: PM2_CNT_BLK not present\n");
		return;
	}

	if (acpi_pm.pm2_cnt_len == 0) {
		printk("ACPI: PM2_CNT_BLK has invalid length\n");
		acpi_pm.pm2_cnt = 0;
		return;
	}

	acpi_pm.has_pm2 = 1;

	printk("ACPI: PM2_CNT_BLK = %08x len=%u\n",
		acpi_pm.pm2_cnt,
		acpi_pm.pm2_cnt_len);
}

static void acpi_parse_pm_timer_blk(struct acpi_fadt *fadt)
{
	acpi_pm.pm_timer = fadt->pm_timer_block;
	acpi_pm.pm_timer_len = fadt->pm_timer_length;

	if (acpi_pm.pm_timer == 0) {
		printk("ACPI: PM Timer not present\n");
		return;
	}

	if (acpi_pm.pm_timer_len != 4) {
		printk("ACPI: invalid PM Timer length: %u\n",
			acpi_pm.pm_timer_len);

		acpi_pm.pm_timer = 0;
		return;
	}

	acpi_pm.has_pm_timer = 1;

	printk("ACPI: PM Timer = %08x\n",
		acpi_pm.pm_timer);
}

static void acpi_parse_gpe0_blk(struct acpi_fadt *fadt)
{
	acpi_pm.gpe0 = fadt->gpe0_block;
	acpi_pm.gpe0_len = fadt->gpe0_length;

	if (acpi_pm.gpe0 == 0 || acpi_pm.gpe0_len == 0) {
		printk("ACPI: GPE0 not present\n");
		return;
	}

	acpi_pm.has_gpe0 = 1;

	printk("ACPI: GPE0 = %08x len=%u\n",
		acpi_pm.gpe0,
		acpi_pm.gpe0_len);
}

static void acpi_parse_gpe1_blk(struct acpi_fadt *fadt)
{
	acpi_pm.gpe1 = fadt->gpe1_block;
	acpi_pm.gpe1_len = fadt->gpe1_length;

	if (acpi_pm.gpe1 == 0 || acpi_pm.gpe1_len == 0) {
		printk("ACPI: GPE1 not present\n");
		return;
	}

	acpi_pm.has_gpe1 = 1;

	printk("ACPI: GPE1 = %08x len=%u\n",
		acpi_pm.gpe1,
		acpi_pm.gpe1_len);
}

static void acpi_parse_reset(struct acpi_fadt *fadt)
{
	if (fadt->reset_register.address == 0) {
		printk("ACPI: reset register not present\n");
		return;
	}

	acpi_pm.has_reset = 1;

	printk("ACPI: Reset register = %016llx\n",
		fadt->reset_register.address);

	printk("ACPI: Reset value = %02x\n",
		fadt->reset_value);
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
}