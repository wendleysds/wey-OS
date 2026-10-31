#include <kernel/device.h>
#include <kernel/init.h>
#include <kernel/interrupt.h>
#include <kernel/printk.h>
#include <device/acpi.h>
#include <def/errno.h>
#include <mm/kheap.h>
#include <mm/iomem.h>
#include <device/tty.h>

#include <io/region.h>

// Registers (Offsets)
#define UART_RBR 0  // In: Receive Buffer (DLAB=0)
#define UART_THR 0  // Out: Transmit Holding (DLAB=0)
#define UART_DLL 0  // Out: Divisor Latch Low (DLAB=1)
#define UART_IER 1  // Out: Interrupt Enable (DLAB=0)
#define UART_DLH 1  // Out: Divisor Latch High (DLAB=1)
#define UART_IIR 2  // In: Interrupt ID Register
#define UART_FCR 2  // Out: FIFO Control Register
#define UART_LCR 3  // Out: Line Control Register
#define UART_MCR 4  // Out: Modem Control Register
#define UART_LSR 5  // In: Line Status Register
#define UART_MSR 6  // In: Modem Status Register

// Registers Flags
#define UART_LCR_DLAB 0x80  // Divisor Latch Access Bit
#define UART_LCR_8N1  0x03  // 8 bits, no parity, 1 stop bit

#define UART_IER_RDA  0x01  // Enable Received Data Available Interrupt
#define UART_IER_THRE 0x02  // Enable Transmitter Holding Register Empty Interrupt
#define UART_IER_RLS  0x04  // Enable Receiver Line Status Interrupt

#define UART_FCR_ENABLE     0x01 // Enable FIFOs
#define UART_FCR_CLEAR_RX   0x02 // Clear FIFO in RX
#define UART_FCR_CLEAR_TX   0x04 // Clear FIFO in TX
#define UART_FCR_TRIGGER_8  0x80 // Trigger 8 bytes level in node RX

#define UART_MCR_DTR  0x01
#define UART_MCR_RTS  0x02
#define UART_MCR_OUT2 0x08

#define UART_IIR_NO_INT 0x01
#define UART_IIR_ID_MASK 0x0E

#define TX_RING_BUFFER_SIZE 256
#define MAX_UART_PORTS 4

struct uart_device {
    io_region_t io;
    uint32_t irq;

    struct tty_struct *tty;

    spinlock_t rx_lock;

        spinlock_t tx_lock;
    u8 tx_buf[TX_RING_BUFFER_SIZE];
    volatile u16 tx_head;
    volatile u16 tx_tail;
};

static struct uart_device* uarts[MAX_UART_PORTS];
static int uart_count = 0;

static struct tty_driver* uart_tty_driver = NULL;

static void uart_set_baud_rate(const io_region_t* io, uint32_t baud) {
    uint16_t divisor = (uint16_t)(115200 / baud);
    
    uint8_t lcr = io_read8(io, UART_LCR);
    io_write8(io, UART_LCR, lcr | UART_LCR_DLAB);
    io_write8(io, UART_DLL, (uint8_t)(divisor & 0xFF));
    io_write8(io, UART_DLH, (uint8_t)((divisor >> 8) & 0xFF));
    io_write8(io, UART_LCR, lcr & ~UART_LCR_DLAB);
}

static void uart_16550A_isr(struct irq_info *info)
{
    struct uart_device *uart = info->device;
    const io_region_t *uart_io = &uart->io;

    while (1) {
        u8 iir = io_read8(uart_io, UART_IIR);

        if (iir & UART_IIR_NO_INT) {
            break;
        }

        switch (iir & UART_IIR_ID_MASK) {
            case 0x06: // Receiver Line Status
                (void)io_read8(uart_io, UART_LSR);
                break;

            case 0x04:
            case 0x0C:
                spin_lock(&uart->rx_lock);
                while (io_read8(uart_io, UART_LSR) & 0x01) {
                    u8 ch = io_read8(uart_io, UART_RBR);
                    if (uart->tty) {
                        spin_unlock(&uart->rx_lock);
                        tty_receive_buf(uart->tty, &ch, 1);
                        spin_lock(&uart->rx_lock);
                    }
                }
                spin_unlock(&uart->rx_lock);
                break;

            case 0x02:
                spin_lock(&uart->tx_lock);
                while (uart->tx_tail != uart->tx_head) {
                    if (!(io_read8(uart_io, UART_LSR) & 0x20)) {
                        break;
                    }
                    io_write8(uart_io, UART_THR, uart->tx_buf[uart->tx_tail]);
                    uart->tx_tail = (uart->tx_tail + 1) % TX_RING_BUFFER_SIZE;
                }

                if (uart->tx_tail == uart->tx_head) {
                    u8 ier = io_read8(uart_io, UART_IER);
                    io_write8(uart_io, UART_IER, ier & ~UART_IER_THRE);
                }
                spin_unlock(&uart->tx_lock);
                break;

            case 0x00: // Modem Status
                (void)io_read8(uart_io, UART_MSR);
                break;
        }
    }
}

static __init int uart_16550A_probe(struct device* dev){
	struct resource *data_port_res = device_get_resource(dev, RESOURCE_TYPE_IO, 0);
	if(!data_port_res) data_port_res = device_get_resource(dev, RESOURCE_TYPE_MEMORY, 0);

	struct resource *irq_res = device_get_resource(dev, RESOURCE_TYPE_IRQ, 0);

	if(!data_port_res || !irq_res) return -ENODEV;

	if(uart_count >= MAX_UART_PORTS){
		return -ENOSPC;
	}

	struct uart_device *uart = kzalloc(sizeof(struct uart_device));
	if(!uart) return -ENOMEM;

    spinlock_init(&uart->rx_lock);
    spinlock_init(&uart->tx_lock);
	
	int irq = 0;
	for (int i = 0; i < 32; i++) {
		if (irq_res->irq & (1 << i)) {
			irq = i;
			break;
		}
	}

	vaddr_t io_base;
	bool is_pio;
	if(data_port_res->type == RESOURCE_TYPE_IO){
		io_base = data_port_res->io.base;
		is_pio = true;
	}else if(data_port_res->type == RESOURCE_TYPE_MEMORY){
		io_base = (vaddr_t)ioremap(
			data_port_res->range.base,
			data_port_res->range.size
		);
		is_pio = false;
	}else{
		return -ENOTSUP;
	}

	uart->irq = irq;

	uart->io.type = is_pio ? IO_TYPE_PIO : IO_TYPE_MMIO;
	if(is_pio){
		uart->io.pio_base = (uint16_t)io_base;
	}else{
		uart->io.mmio_base = io_base;
	}

	const io_region_t* uart_io = &uart->io;

	// Hardware init
	io_write8(uart_io, UART_IER, 0);
	uart_set_baud_rate(uart_io, 115200);
	io_write8(uart_io, UART_LCR, UART_LCR_8N1);
	io_write8(uart_io, UART_FCR, UART_FCR_ENABLE | UART_FCR_CLEAR_RX | UART_FCR_CLEAR_TX | UART_FCR_TRIGGER_8);
    io_write8(uart_io, UART_MCR, UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2);

	if (irq > 0) {
		int err = interrupt_register(irq, uart_16550A_isr, uart);
		if(err){
			kfree(uart);
			if(!is_pio) iounmap((void*)io_base);
			return err;
		}
        
        io_write8(uart_io, UART_IER, UART_IER_RDA | UART_IER_RLS);
    }

	int idx = uart_count++;

	uarts[idx] = uart;
	struct tty_struct* tty = tty_ensure_created(uart_tty_driver, idx);
	if(IS_ERR(tty)){
		kfree(uart);
		if(!is_pio) iounmap((void*)io_base);
		if (irq > 0){
			interrupt_unregister(irq, uart_16550A_isr, uart);
		}

		uarts[--uart_count] = NULL;
		return PTR_ERR(tty);
	}

	printk("UART: Serial ttyS%d initialized\n", idx);

	return 0;
}

static int uart_tty_install(struct tty_driver *self, struct tty_struct *new_tty) {
    if (!self || !new_tty) return -EINVAL;
    if (new_tty->index < 0 || new_tty->index >= uart_count){
		return -EINVAL;
	}

    struct uart_device *uart = uarts[new_tty->index];

    new_tty->private = uart;
    uart->tty = new_tty;

    return 0;
}

static int uart_tty_write(struct tty_struct *self, const u8 *buffer, size_t count) {
    if (!self || !buffer || count == 0) return 0;

    struct uart_device *uart = uarts[self->index];
    if (!uart) return -EINVAL;

    spin_lock(&uart->tx_lock);

	const io_region_t* uart_io = &uart->io;

    size_t written = 0;
    while (written < count) {
        u16 next = (uart->tx_head + 1) % TX_RING_BUFFER_SIZE;
        if (next == uart->tx_tail) {
            break;
        }

        uart->tx_buf[uart->tx_head] = buffer[written];
        uart->tx_head = next;
        written++;
    }

    if (written > 0) {
        u8 ier = io_read8(uart_io, UART_IER);
        if (!(ier & UART_IER_THRE)) {
            io_write8(uart_io, UART_IER, ier | UART_IER_THRE);
        }
    }

    spin_unlock(&uart->tx_lock);

    return (int)written;
}

static const struct acpi_device_id uart_16550A_ids[] = {
	{"PNP0501"},
	{},
};

static struct acpi_driver uart_16550A_driver = {
	.driver = {
		.name = "UART-16550A",
		.probe = uart_16550A_probe,
		.remove = 0,
	},
	.id_table = uart_16550A_ids,
};

static const struct tty_ops uart_tty_ops = {
    .install = uart_tty_install,
    .write   = uart_tty_write
};

static __init int uart_16550A_driver_register(void){
	uart_tty_driver = tty_alloc_driver();
    if (!uart_tty_driver) return -ENOMEM;

    uart_tty_driver->name = "ttyS";
    uart_tty_driver->ops = &uart_tty_ops;
    uart_tty_driver->major = 4;
    uart_tty_driver->minor_start = 64;
    uart_tty_driver->num = MAX_UART_PORTS;

	int res = tty_register_driver(uart_tty_driver);
	if(res){
		kfree(uart_tty_driver);
	}

	return acpi_register_driver(&uart_16550A_driver);
}

device_initcall(uart_16550A_driver_register);
