#include <def/config.h>
#include <def/errno.h>
#include <kernel/clock.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <kernel/acpi.h>
#include <kernel/panic.h>

#include <kernel/acpi/tables/hpet.h>
#include <kernel/acpi/tables/madt.h>

#include <lib/string.h>

#include <arch/i386/pic.h>
#include <arch/i386/ioapic.h>
#include <arch/i386/timer_caps.h>

extern struct acpi_madt_info *acpi_madt_info;

static __init void x86_discover_timer_caps(
        struct x86_timer_caps *caps
){
    memset(caps, 0, sizeof(*caps));

    caps->hpet_available =
        !!acpi_find_table(ACPI_HPET_SIGNATURE);

    caps->lapic_available = !!acpi_madt_info;
    caps->ioapic_available =
        acpi_madt_info && acpi_madt_info->ioapic_count > 0;

    // Legacy fallbacy
    caps->pic_legacy =
        !acpi_madt_info ||
        !!(acpi_madt_info->flags & MADT_FLAG_PCAT_COMPAT);

    caps->pit_available = true;
}

static __init int x86_clocksource_init(
    const struct x86_timer_caps *caps)
{
    int ret;

    if (caps->hpet_available) {
        ret = hpet_init();
        if (ret < 0) {
            printk("x86: HPET initialization failed: %d\n", ret);
        } else {
            ret = clocksource_register(&hpet_clocksource);
            if (ret < 0)
                printk("x86: HPET clocksource registration failed: %d\n", ret);
            else
                return 0;
        }
    }

    return -ENODEV;
}

static __init int x86_early_parse_madt(void){
    int ret;
    if((ret = acpi_load_all_tables()) < 0){
		printk("x86: Failed to load ACPI tables: %d\n", ret);
		return ret;
	}

	struct acpi_madt* madt = (void*)acpi_find_table(ACPI_MADT_SIGNATURE);
	if(!madt){
        printk("x86: MADT not found\n");
        return -ENODEV;
    }

    acpi_parse_madt(madt);
    return 0;
}

extern int x86_irqchip_init(struct x86_timer_caps* caps);
int __init arch_setup_clock(void){
    int ret = x86_early_parse_madt();
    if(ret < 0){
        printk("x86: Failed to parse MADT: %d\n", ret);
    }

    struct x86_timer_caps caps;

    x86_discover_timer_caps(&caps);

    ret = x86_clocksource_init(&caps);
    if (ret < 0)
        printk("x86: no hardware clocksource available, using software based\n");

    ret = x86_irqchip_init(&caps);
    if (ret < 0)
        return ret;

    return 0;
}