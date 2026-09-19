#include <kernel/interrupt.h>
#include <kernel/init.h>

#define PORT_NEW_API
#include <io/ports.h>

#define PIC1          0x20
#define PIC2          0xA0

#define PIC1_COMMAND  PIC1
#define PIC1_DATA     (PIC1+1)
#define PIC2_COMMAND  PIC2
#define PIC2_DATA     (PIC2+1)

#define PIC_EOI       0x20
#define PIC_CHANNEL0  0x40
#define PIC_COMMAND   0x43
#define PIC_FREQUENCY 1193182
#define PIC_READ_IRR  0x0A
#define PIC_READ_ISR  0x0B

static void __init pic_remap(void){
    port_write8(PIC1_COMMAND, 0x11);
    port_write8(PIC2_COMMAND, 0x11);

    port_write8(PIC1_DATA, 0x20);
    port_write8(PIC2_DATA, 0x28);

    port_write8(PIC1_DATA, 0x04);
    port_write8(PIC2_DATA, 0x02);

    port_write8(PIC1_DATA, 0x01);
    port_write8(PIC2_DATA, 0x01);
}

static uint16_t __pic_get_irq_reg(int ocw3)
{
    port_write8(PIC1_COMMAND, ocw3);
    port_write8(PIC2_COMMAND, ocw3);
    return (port_read8(PIC2_COMMAND) << 8) | port_read8(PIC1_COMMAND);
}

static uint16_t pic_get_irr()
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}

static uint16_t pic_get_isr()
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}

// IRQ7 handler (slave PIC spurious check)
static void _irq7_handler() {
    uint8_t isr = pic_get_isr();

    if (!(isr & (1 << 7))) {
        return;
    }

    port_write8(PIC1_COMMAND, PIC_EOI);
}

// IRQ15 handler (slave PIC spurious check)
static void _irq15_handler() {
    uint8_t isr = pic_get_isr();

    if (!(isr & (1 << 7))) {
        port_write8(PIC1_COMMAND, PIC_EOI);
        return;
    }
    
    port_write8(PIC2_COMMAND, PIC_EOI);
    port_write8(PIC1_COMMAND, PIC_EOI);
}

static void __init pic_init(int frequency) {
    pic_remap();

    uint16_t divisor = (uint16_t)(PIC_FREQUENCY / frequency);

    port_write8(PIC_CHANNEL0, 0x36);                  // Channel 0, lobyte/hibyte
    port_write8(PIC_COMMAND, divisor & 0xFF);         // low end
    port_write8(PIC_CHANNEL0, (divisor >> 8) & 0xFF); // high end
}

static void pic_send_eoi(int irq)
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
        port_write8(PIC2_COMMAND, PIC_EOI);
    }
    
    port_write8(PIC1_COMMAND, PIC_EOI);
}

static void pic_disable(int irq) {
    port_write8(PIC1_DATA, 0xff);
    port_write8(PIC2_DATA, 0xff);
}

static void pic_enable(int irq) {
    port_write8(PIC1_DATA, 0x0);
    port_write8(PIC2_DATA, 0x0);
}

static void IRQ_set_mask(int IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = port_read8(port) | (1 << IRQline);
    port_write8(port, value);        
}

static void IRQ_clear_mask(int IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = port_read8(port) & ~(1 << IRQline);
    port_write8(port, value);        
}

const struct irq_chip i8259A_chip = {
    .name = "i8259A",
    .init = pic_init,
    .eoi = pic_send_eoi,
    .disable = pic_disable,
    .enable = pic_enable,
    .mask = IRQ_set_mask,
    .unmask = IRQ_clear_mask,
};
