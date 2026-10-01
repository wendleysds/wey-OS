#include <kernel/sched.h>
#include <kernel/clock.h>
#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <kernel/stacktrace.h>
#include <kernel/panic.h>
#include <mm/kheap.h>
#include <asm/idt.h>
#include <def/errno.h>
#include <lib/string.h>

#include <kernel/acpi/tables/madt.h>
#include <kernel/acpi/tables/hpet.h>

#include <arch/i386/pic.h>
#include <arch/i386/lapic.h>
#include <arch/i386/ioapic.h>

static const char* exception_messages[] = {
	"Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
	"Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
	"Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
	"Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt",
	"x87 Floating-Point", "Alignment Fault", "Machine Check", "SIMD Floating-Point",
	"Vitualization",
	"Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

extern void* interrupt_pointer_table[TOTAL_INTERRUPTS];

static struct InterruptDescriptor idt[TOTAL_INTERRUPTS];
static struct IDTr_ptr idtr_ptr;

static struct irq_desc irq_table[TOTAL_INTERRUPTS];

extern void kernel_registers();
extern void user_registers();

static inline void _idt_load(struct IDTr_ptr* ptr){
	__asm__ volatile ("lidt (%0)" : : "r"(ptr));
}

void idt_set_gate(uint8_t interrupt_num, uint32_t base, uint16_t selector, uint8_t flags){
	struct InterruptDescriptor* desc = &idt[interrupt_num];
	desc->offset_1 = base & 0xFFFF;
	desc->selector = selector;
	desc->zero = 0x0;
	desc->type_attributes = flags;
	desc->offset_2 = (base >> 16) & 0xFFFF;
}

__init void idt_init(){
	memset(irq_table, 0x0, sizeof(irq_table));
	memset(idt, 0x0, sizeof(idt));
	idtr_ptr.limit = sizeof(idt) - 1;
	idtr_ptr.base = (uintptr_t)&idt;

	for(int i = 0; i < TOTAL_INTERRUPTS; i++){
		idt_set_gate(
			i, 
			(uint32_t)interrupt_pointer_table[i], 
			GDT_KERNEL_CODE, 
			(IDT_PRESENT | IDT_DPL0 | IDT_TYPE_INT_GATE32)
		);
	}

	_idt_load(&idtr_ptr);
	printk("Setup: idt: loaded \"%#lx\".\n", &idtr_ptr);
}

void interrupts_enable(){
	__asm__ volatile ("sti");
}

void interrupts_disable(){
	__asm__ volatile ("cli");
}

static void _build_irq_info(struct irq_info* info, struct registers* regs){
	info->cpu.cpu_id = 0;
	info->cpu.regs = regs;
	info->cpu.from_user = regs_is_user_mode(regs);
	info->hwirq = regs->int_no;

	if(regs->int_no < 0x20){
		info->cpu.exception = true;
		info->cpu.exception_name = exception_messages[regs->int_no];
	}

	info->needs_eoi = (regs->int_no > 0x20 && !info->cpu.from_user);
}

asmlinkage void arch_handle_irq(struct registers* regs){
	if(likely(current))
		current->regs = *regs;

	struct irq_info info;
	memset(&info, 0x0, sizeof(info));

	_build_irq_info(&info, regs);

	generic_handle_irq(&info);

	if(info.hwirq == 0x20){
		clockevent_fire();
	}

	if(likely(current)){
		*regs = current->regs;
	}
}

static __init void hpet_10ms_wait(void) {
	hpet_udelay(10000);
}

static __init void pit_10ms_wait(void){
	const uint32_t pit_hz = 100;
	const uint16_t pit_ticks = PIT_FREQUENCY / pit_hz;

	// Problaby the period is not very accurate 
	// for high precision delays
	pit_prepare_oneshot_ch2(pit_ticks);
	pit_wait_oneshot_ch2();
}

static __init int irq_use_apic_and_hpet(void){
	int res = 0;
	printk("x86: using APIC + HPET.\n");

	res = hpet_init();
	if(res < 0){
		printk("x86: Failed to initialize HPET: %d\n", res);
		return res;
	}

	res = lapic_init(TIMER_FREQUENCY_HZ, hpet_10ms_wait);
	if(res < 0){
		printk("x86: Failed to initialize APIC: %d\n", res);
		return res;
	}

	res = ioapic_init(lapic_get_id());
	if(res < 0){
		printk("x86: Failed to initialize IOAPIC: %d\n", res);
		return res;
	}

	irq_switch_all_chips(&ioapic_chip, &ioapic_controller);
	
	return res;
}

static __init int irq_use_apic_and_pic(void){
	int res = 0;
	printk("x86: using APIC + PIC.\n");

	res = pic_init(TIMER_FREQUENCY_HZ);
	if(res < 0){
		return res;
	}

	res = lapic_init(TIMER_FREQUENCY_HZ, pit_10ms_wait);
	if(res < 0){
		return res;
	}

	res = ioapic_init(lapic_get_id());
	if(res < 0){
		return res;
	}

	irq_switch_all_chips(&ioapic_chip, &ioapic_controller);

	return 0;
}

static __init int irq_use_pic(void){
	int res = 0;
	printk("x86: using PIC.\n");
	res = pic_init(TIMER_FREQUENCY_HZ);
	if(res < 0){
		return res;
	}

	irq_switch_all_chips(&i8259A_chip, &i8259A_controller);

	return 0;
}

static void __init irqdesc_setup(int irq, struct irq_desc *desc){
	if(irq >= 0x20 && irq <= 0x2F){
		desc->irq = irq - 0x20;
	}else{
		desc->irq = irq;
	}

	desc->hwirq = irq;
	desc->masked = true;
}

static int __init irq_check_chips(void){
	bool has_pic = false;
	bool has_hpet = !!acpi_find_table(ACPI_HPET_SIGNATURE);

	int res;

	if(!has_pic && !acpi_madt_info){
		panic("No IRQ chip found!");
	}

	if(acpi_madt_info){
		has_pic |= (acpi_madt_info->flags & MADT_FLAG_PCAT_COMPAT);
	}

	if(!has_pic && !has_hpet){
		return -ENODEV;
	}

	irqdesc_init(irqdesc_setup);
	if(has_hpet && acpi_madt_info){
		res = irq_use_apic_and_hpet();
		if (res >= 0) return res;
	}

	if(has_pic && acpi_madt_info){
		res = irq_use_apic_and_pic();
		if (res >= 0) return res;
	}

	if(has_pic){
		res = irq_use_pic();
		if (res >= 0) return res;
	}

	panic("x86: only support APIC + PIC.");
}

int __init interrupt_init(void){
	int res;

	if((res = acpi_load_all_tables()) < 0){
		printk("x86: Failed to load ACPI tables: %d\n", res);
		return res;
	}

	struct acpi_madt* madt = (void*)acpi_find_table(ACPI_MADT_SIGNATURE);
	if(madt){
		acpi_parse_madt(madt);
	}
	
	if((res = irq_check_chips()) < 0){
		printk("x86: Failed to setup IRQ chips: %d\n", res);
		return res;
	}

	// Handle overrides if exists

	return 0;
}

