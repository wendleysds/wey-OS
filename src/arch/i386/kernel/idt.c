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
#include <arch/i386/timer_caps.h>

#define X86_IRQ_MODE_APIC  0
#define X86_IRQ_MODE_PIC   1
#define X86_IRQ_MODE_UNKNOWN -1

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
static int x86_irq_mode = X86_IRQ_MODE_UNKNOWN;

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
	const uint16_t pit_ticks = PIT_FREQUENCY / 100;
	pit_prepare_oneshot_ch2(pit_ticks);
	pit_wait_oneshot_ch2();
}

static __init wait_10ms_func_t select_calibrate_func(struct x86_timer_caps* caps){
	if(hpet_is_initialized())
        return hpet_10ms_wait;

    if (caps->pit_available)
        return pit_10ms_wait;

    return NULL;
}

static __init int lapic_timer_prepare(){
    int ret;

	ret = lapic_prepare_init();
	if(ret < 0){
		printk("x86: LAPIC timer prepare failed: %d\n", ret);
		return ret;
	}

    return 0;
}

int x86_irqchip_init(struct x86_timer_caps* caps){
	int ret;

	bool has_lapic = caps->lapic_available && lapic_supporte();
	if (has_lapic && caps->ioapic_available) {
		wait_10ms_func_t wait_fn = select_calibrate_func(caps);

		if (wait_fn) {
			ret = lapic_timer_prepare();
			if (ret >= 0) {
				if(caps->pic_legacy){
					pic_disable();
				}

				lapic_init(TIMER_FREQUENCY_HZ, wait_fn);

				ret = ioapic_init(lapic_get_id());
				if (ret >= 0) {
					x86_irq_mode = X86_IRQ_MODE_APIC;
					return 0;
				}else{
					if(caps->pic_legacy){
						pic_enable();
					}
				}
			}
		}

		printk("x86: APIC setup failed, trying legacy PIC\n");
	}

	if (caps->pic_legacy) {
		ret = pic_irqchip_init();
		if (ret < 0)
			return ret;

		x86_irq_mode = X86_IRQ_MODE_PIC;
		return 0;
	}

	return -ENODEV;
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

static __init int x86_clockevent_start(void)
{
	const struct clockevent *ce;
	int ret;

	switch (x86_irq_mode) {
	case X86_IRQ_MODE_APIC:
		ce = &lapic_clockevent;
		break;

	case X86_IRQ_MODE_PIC:
		ce = &i8259a_clockevent;
		break;

	default:
		return -ENODEV;
	}

	interrupt_unmask(0);

	ret = clockevent_register(ce);
	if (ret < 0)
		return ret;

	ret = clockevent_start_periodic(TIMER_FREQUENCY_HZ);
	if (ret < 0)
		return ret;

	printk("x86: clockevent started! %s @ %u Hz\n",
		x86_irq_mode == X86_IRQ_MODE_APIC ? "APIC" : "PIC",
		TIMER_FREQUENCY_HZ
	);

	return 0;
}

static __init int interrupt_setup_chips(void){
	switch(x86_irq_mode){
		case X86_IRQ_MODE_APIC:
			return irq_switch_all_chips(&ioapic_chip, &ioapic_controller);
		case X86_IRQ_MODE_PIC:
			return irq_switch_all_chips(&i8259A_chip, &i8259A_controller);
		default:
			return -ENODEV;
	}
}

int __init interrupt_init(void){
	int ret;	

	ret = irqdesc_init(irqdesc_setup);
	if(ret < 0){
		printk("x86: irqdesc init failed: %d\n", ret);
		return ret;
	}

	ret = interrupt_setup_chips();
	if(ret < 0){
		printk("x86: interrupt setup failed: %d\n", ret);
		return ret;
	}

	// Handle overrides if exists
	// TODO: Implement Irq overrides

	// Always called after irq vector have be setup
	ret = x86_clockevent_start();
	if (ret < 0)
		return ret;

	return 0;
}
