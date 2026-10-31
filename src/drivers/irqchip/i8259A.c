#include <kernel/interrupt.h>
#include <kernel/init.h>
#include <kernel/clock.h>
#include <def/bits.h>
#include <def/errno.h>

#include <arch/i386/pic.h>

#define PORT_NEW_API
#include <io/ports.h>

#define PIC1_BASE         0x20
#define PIC2_BASE         0xA0

extern const struct irq_chip i8259A_chip;
extern const struct irq_controller i8259A_controller;

static bool pic_remaped = false;
static bool pic_running = false;

struct pic_info master = {
    .cmd_port = PIC1_BASE,
    .data_port = PIC1_BASE + 1,
    .irq_base = 0x20,
    .current_frequency = PIT_FREQUENCY,
};

struct pic_info slave = {
    .cmd_port = PIC2_BASE,
    .data_port = PIC2_BASE + 1,
    .irq_base = 0x28,
    .current_frequency = PIT_FREQUENCY,
};

void pic_write_cmd(struct pic_info *pic, uint8_t cmd) {
    port_write8(pic->cmd_port, cmd);
}

void pic_write_data(struct pic_info *pic, uint8_t data) {
    port_write8(pic->data_port, data);
}

uint8_t pic_read_cmd(struct pic_info *pic) {
    return port_read8(pic->cmd_port);
}

uint8_t pic_read_data(struct pic_info *pic) {
    return port_read8(pic->data_port);
}

static inline void pic_send_cmd_eoi(struct pic_info *pic) {
    pic_write_cmd(pic, PIC_CMD_EOI);
}

static inline void pic_mask_irq(struct pic_info *pic, uint8_t offset) {
    uint8_t mask = pic_read_data(pic) | (1 << offset);
    pic_write_data(pic, mask);
}

static inline void pic_unmask_irq(struct pic_info *pic, uint8_t offset) {
    uint8_t mask = pic_read_data(pic) & ~(1 << offset);
    pic_write_data(pic, mask);
}

static uint8_t pic_get_irq_reg(struct pic_info *pic, uint8_t ocw3) {
    pic_write_cmd(pic, ocw3);
    return pic_read_cmd(pic);
}

static void __init pic_remap(void) {
    if(pic_remaped)
        return;

    pic_remaped = true;

    pic_write_cmd(&master, 0x11);
    pic_write_cmd(&slave, 0x11);

    pic_write_data(&master, master.irq_base);
    pic_write_data(&slave, slave.irq_base);

    pic_write_data(&master, 0x04);
    pic_write_data(&slave, 0x02);

    pic_write_data(&master, 0x01);
    pic_write_data(&slave, 0x01);
}

static uint16_t pic_get_irr(void) {
    return ((uint16_t)pic_get_irq_reg(&slave, PIC_CMD_READ_IRR) << 8) | pic_get_irq_reg(&master, PIC_CMD_READ_IRR);
}

static uint16_t pic_get_isr(void) {
    return ((uint16_t)pic_get_irq_reg(&slave, PIC_CMD_READ_ISR) << 8) | pic_get_irq_reg(&master, PIC_CMD_READ_ISR);
}

static void _irq7_handler(void) {
    uint8_t isr = pic_get_irq_reg(&master, PIC_CMD_READ_ISR);

    if (!(isr & (1 << 7))) {
        return;
    }

    pic_send_cmd_eoi(&master);
}

static void _irq15_handler(void) {
    uint8_t isr = pic_get_irq_reg(&slave, PIC_CMD_READ_ISR);

    if (!(isr & (1 << 7))) {
        pic_send_cmd_eoi(&master);
        return;
    }

    pic_send_cmd_eoi(&slave);
    pic_send_cmd_eoi(&master);
}

void pic_disable(void) {
    pic_write_data(&master, 0xFF);
    pic_write_data(&slave, 0xFF);
}

void pic_enable(void) {
    pic_write_data(&master, 0x00);
    pic_write_data(&slave, 0x00);
}

void pit_prepare_oneshot_ch2(uint16_t ticks) {
    /* PIT channel 2, mode 0, binary */
    port_write8(PIT_COMMAND, 0xB0);
    port_write8(PIT_CHANNEL2, ticks & 0xFF);
    port_write8(PIT_CHANNEL2, ticks >> 8);

    /* Enable PIT channel 2 gate */
    uint8_t gate = port_read8(PIT_GATE_PORT);
    gate = (gate & ~BIT(1)) | BIT(0);
    port_write8(PIT_GATE_PORT, gate);
}

void pit_wait_oneshot_ch2(void) {
    while (!(port_read8(PIT_GATE_PORT) & BIT(5)))
        asm volatile("pause");
}

static void pic_send_eoi(struct irq_desc* desc) {
	int hwirq = desc->hwirq;

    if (hwirq == 7) {
        _irq7_handler();
        return;
    }

    if (hwirq == 15) {
        _irq15_handler();
        return;
    }

    if (hwirq >= 8) {
        pic_send_cmd_eoi(&slave);
    }

    pic_send_cmd_eoi(&master);
}

static void pic_mask(struct irq_desc* desc) {
	int hwline = desc->hwirq;

    if (hwline < 8) {
        pic_mask_irq(&master, hwline);
    } else {
        pic_mask_irq(&slave, hwline - 8);
    }
}

static void pic_unmask(struct irq_desc* desc) {
	int hwline = desc->hwirq;

    if (hwline < 8) {
        pic_unmask_irq(&master, hwline);
    } else {
        pic_unmask_irq(&slave, hwline - 8);
    }
}

void pic_start_periodic(uint32_t hz){
	if(pic_running) return;

    pic_running = true;
	uint16_t divisor = (uint16_t)(PIT_FREQUENCY / hz);

	port_write8(PIT_COMMAND, 0x36); /* canal 0, modo 3 (square wave), binary */
	port_write8(PIT_CHANNEL0, divisor & 0xFF);
	port_write8(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

	master.current_frequency = (int)hz;
	slave.current_frequency  = (int)hz;

    uint8_t mask = pic_read_data(&master) & ~(1 << 0);
	pic_write_data(&master, mask);
}

// PIC as IRQCHIP
int __init pic_irqchip_init() {
    pic_remap();

    irq_domain_create_legacy("i8259A", 16, master.irq_base, 0, &i8259A_chip, &i8259A_controller);

    return SUCCESS;
}

const struct irq_chip i8259A_chip = {
    .name = "i8259A",
    .mask = pic_mask,
    .unmask = pic_unmask,
};

const struct irq_controller i8259A_controller = {
    .name = "i8259A",
    .eoi = pic_send_eoi,
};

// PIC as Clock
static int i8259a_clockevent_start_periodic(void *data, uint32_t hz){
	if(!hz || hz > PIT_FREQUENCY)
		return -EINVAL;

    // Necessary to not conflict with cpu exceptions
    pic_remap();

	pic_start_periodic(hz);

	return 0;
}

static void i8259a_clockevent_stop(void *data){
	uint8_t mask = pic_read_data(&master) | (1 << 0);
	pic_write_data(&master, mask);
    pic_running = false;
}

const struct clockevent i8259a_clockevent = {
	.name           = "pit-i8259A",
	.start_periodic = i8259a_clockevent_start_periodic,
	.stop           = i8259a_clockevent_stop,
	.data           = 0x0,
};
