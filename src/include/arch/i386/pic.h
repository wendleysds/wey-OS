#ifndef _I386_PIC_H
#define _I386_PIC_H

#include <stdint.h>

#define PIC_CMD_EOI       0x20
#define PIC_CMD_READ_IRR  0x0A
#define PIC_CMD_READ_ISR  0x0B

#define PIT_CHANNEL0      0x40
#define PIT_CHANNEL2      0x42
#define PIT_COMMAND       0x43
#define PIT_GATE_PORT     0x61
#define PIT_FREQUENCY     1193182

struct pic_info {
    uint16_t cmd_port;
    uint16_t data_port;
    int irq_base;
    int current_frequency;
};

extern const struct irq_chip i8259A_chip;
extern struct pic_info master;
extern struct pic_info slave;

int pic_init(int frequency);
void pic_disable(void);
void pic_enable(void);

void pic_write_cmd(struct pic_info *pic, uint8_t cmd);
void pic_write_data(struct pic_info *pic, uint8_t data);
uint8_t pic_read_cmd(struct pic_info *pic);
uint8_t pic_read_data(struct pic_info *pic);

void pit_prepare_oneshot_ch2(uint16_t ticks);
void pit_wait_oneshot_ch2(void);

#endif /* _I386_PIC_H */