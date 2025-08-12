#include <arch/i386/pic.h>
#include <arch/i386/idt.h>
#include <io/ports.h>
#include <def/config.h>
#include <stdint.h>
#include <drivers/terminal.h>

#define PIC1		0x20
#define PIC2		0xA0
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define PIC_EOI       0x20
#define PIC_CHANNEL0  0x40
#define PIC_COMMAND   0x43
#define PIC_FREQUENCY 1193182

#define PIC_READ_IRR 0x0a
#define PIC_READ_ISR 0x0b

extern void pic_remap();

uint64_t spuriousIrqCount = 0;

static uint16_t __pic_get_irq_reg(int ocw3)
{
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

static void _timer_iqr_handler(struct InterruptFrame* frame){
	outb(PIC1_COMMAND, PIC_EOI);
}

// IRQ7 handler (slave PIC spurious check)
static void _irq7_handler() {
    uint8_t isr = pic_get_isr();

    if (!(isr & (1 << 7))) {
        spuriousIrqCount++;
        return;
    }

    outb(PIC1_COMMAND, PIC_EOI);
}

// IRQ15 handler (slave PIC spurious check)
static void _irq15_handler() {
    uint8_t isr = pic_get_isr();

    if (!(isr & (1 << 7))) {
        spuriousIrqCount++;
        outb(PIC1_COMMAND, PIC_EOI);
        return;
    }
    
    outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_init(uint32_t frequency) {
	pic_remap();

	uint16_t divisor = (uint16_t)(PIC_FREQUENCY / frequency);

	outb(PIC_CHANNEL0, 0x36);                  // Channel 0, lobyte/hibyte
	outb(PIC_COMMAND, divisor & 0xFF);         // low end
	outb(PIC_CHANNEL0, (divisor >> 8) & 0xFF); // high end

	idt_register_callback(0x20, _timer_iqr_handler);
}

void pic_send_eoi(uint8_t irq)
{
    if(irq == 7){
        _irq7_handler();
        return;
    }

    if(irq == 15){
        _irq15_handler();
        return;
    }

	if(irq >= 8){
		outb(PIC2_COMMAND, PIC_EOI);
	}
	
	outb(PIC1_COMMAND, PIC_EOI);
}

void pic_disable() {
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void IRQ_set_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);        
}

void IRQ_clear_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);        
}

uint16_t pic_get_irr()
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}

uint16_t pic_get_isr()
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}