#include <kernel/interrupt.h>
#include <kernel/init.h>
#include <kernel/printk.h>
#include <kernel/acpi/tables/madt.h>

#include <arch/i386/lapic.h>
#include <arch/i386/pic.h>

#define PORT_NEW_API
#include <io/ports.h>
#include <io/mmio.h>
#include <mm/iomem.h>
#include <def/errno.h>

#include <stdint.h>
#include <stdbool.h>

#define LAPIC_ID          0x020
#define LAPIC_VER         0x030
#define LAPIC_TPR         0x080 // Task Priority Register
#define LAPIC_EOI         0x0B0 // End of Interrupt
#define LAPIC_LDR         0x0D0 // Logical Destination Register
#define LAPIC_DFR         0x0E0 // Destination Format Register
#define LAPIC_SVR         0x0F0 // Spurious Interrupt Vector Register
#define LAPIC_ESR         0x280 // Error Status Register
#define LAPIC_ICR_LOW     0x300 // Interrupt Command Register
#define LAPIC_ICR_HIGH    0x310
#define LAPIC_LVT_TIMER   0x320 // LVT Timer Register
#define LAPIC_LVT_LINT0   0x350 // Local Vector Table LINT0
#define LAPIC_LVT_LINT1   0x360 // Local Vector Table LINT1
#define LAPIC_LVT_ERROR   0x370 // LVT Error Register
#define LAPIC_TICR        0x380 // Timer Initial Count Register
#define LAPIC_TCCR        0x390 // Timer Current Count Register
#define LAPIC_TDCR        0x3E0 // Timer Divide Configuration Register

#define LAPIC_SPURIOUS_VECTOR   0xFF
#define LAPIC_SVR_ENABLE        BIT(8)
#define LAPIC_TIMER_DIV_16      0x03

#define LAPIC_LVT_MASK          BIT(16) // Bit 16 = Masked
#define LAPIC_TIMER_PERIODIC    BIT(17) // Bit 17 = Periodic Mode

#define IA32_APIC_BASE_MSR      0x1B
#define IA32_APIC_BASE_ENABLE   BIT(11)
#define IA32_APIC_BASE_X2APIC   BIT(10)

#define LAPIC_TIMER_VECTOR      0x20

extern struct acpi_madt_info *acpi_madt_info;
static vaddr_t __iomem lapic_base = 0;
static uint32_t lapic_bus_freq_hz = 0;

static inline uint32_t lapic_read(uint32_t reg) {
    return mmio_read32(lapic_base + reg);
}

static inline void lapic_write(uint32_t reg, uint32_t value) {
    mmio_write32(lapic_base + reg, value);
    (void)lapic_read(LAPIC_ID); 
}

static int cpuid(int leaf, int *eax, int *ebx, int *ecx, int *edx){
    asm volatile(
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (leaf)
    );
    return 0;
}

static inline uint64_t rdmsr(uint32_t msr){
    uint32_t lo, hi;
    asm volatile (
        "rdmsr"
        : "=a"(lo), "=d"(hi)
        : "c"(msr)
    );
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t value){
    uint32_t lo = (uint32_t)value;
    uint32_t hi = (uint32_t)(value >> 32);
    asm volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(lo), "d"(hi)
        : "memory"
    );
}

static bool cpu_supports_apic(void){
    int eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    return BIT_CHECK(edx, 9);
}

static bool cpu_supports_msr(void){
    int eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    return BIT_CHECK(edx, 5);
}

static void lapic_hw_enable(void){
    uint64_t value = rdmsr(IA32_APIC_BASE_MSR);
    value |= IA32_APIC_BASE_ENABLE;
    wrmsr(IA32_APIC_BASE_MSR, value);
}

void lapic_enable(void){
    uint32_t svr = lapic_read(LAPIC_SVR);
    svr |= LAPIC_SVR_ENABLE;
    lapic_write(LAPIC_SVR, svr);
}

void lapic_disable(void){
    uint32_t svr = lapic_read(LAPIC_SVR);
    svr &= ~LAPIC_SVR_ENABLE;
    lapic_write(LAPIC_SVR, svr);
}

void lapic_mask(uint32_t lvt_reg){
    uint32_t val = lapic_read(lvt_reg);
    val |= LAPIC_LVT_MASK;
    lapic_write(lvt_reg, val);
}

void lapic_unmask(uint32_t lvt_reg){
    uint32_t val = lapic_read(lvt_reg);
    val &= ~LAPIC_LVT_MASK;
    lapic_write(lvt_reg, val);
}

void lapic_mask_local_sources(void){
    lapic_mask(LAPIC_LVT_LINT0);
    lapic_mask(LAPIC_LVT_LINT1);
    lapic_mask(LAPIC_LVT_ERROR);
}

void lapic_eoi(void){
    lapic_write(LAPIC_EOI, 0);
}

uint32_t lapic_get_id(void){
    if(!lapic_base) return 0;
    return (lapic_read(LAPIC_ID) >> 24) & 0xFF;
}

static void lapic_send_eoi(struct irq_desc *desc){
    lapic_eoi();
}

static __init uint32_t lapic_calibrate(wait_10ms_func_t calibrator){
    lapic_write(LAPIC_TDCR, LAPIC_TIMER_DIV_16);

	lapic_write(LAPIC_TICR, 0xFFFFFFFF);

	calibrator();

	uint32_t elapsed = 0xFFFFFFFF - lapic_read(LAPIC_TCCR);

	lapic_write(LAPIC_TICR, 0);

	return elapsed * 100;
}

static __init void lapic_timer_init(uint32_t target_frequency_hz, wait_10ms_func_t wait_func)
{
    if (!target_frequency_hz)
        return;

    lapic_bus_freq_hz = lapic_calibrate(wait_func);

    uint32_t initial_count =
        lapic_bus_freq_hz / target_frequency_hz;

    printk(
        "LAPIC: Timer frequency: %u Hz, initial count for %u Hz: %u\n",
        lapic_bus_freq_hz,
        target_frequency_hz,
        initial_count
    );

    lapic_write(LAPIC_TDCR, LAPIC_TIMER_DIV_16);

    lapic_write(
        LAPIC_LVT_TIMER,
        LAPIC_TIMER_VECTOR | LAPIC_TIMER_PERIODIC
    );

    lapic_write(LAPIC_TICR, initial_count);
}

int __init lapic_init(int frequency, wait_10ms_func_t wait_func){
    if(!acpi_madt_info){
        printk("LAPIC: No MADT info\n");
        return -ENODEV;
    }

    if(!cpu_supports_msr()){
        printk("LAPIC: CPU does not support MSR\n");
        return -ENODEV;
    }

    if(!cpu_supports_apic()){
        printk("LAPIC: CPU does not support APIC\n");
        return -ENODEV;
    }

    lapic_base = (vaddr_t)ioremap(acpi_madt_info->local_apic_address, PAGE_SIZE);
    if(!lapic_base) return -ENOMEM;

    pic_disable();

    lapic_hw_enable();

    lapic_write(LAPIC_DFR, 0xFFFFFFFF);
    lapic_write(LAPIC_LDR, (lapic_read(LAPIC_LDR) & 0x00FFFFFF) | 1);

    uint32_t svr = lapic_read(LAPIC_SVR);
    svr = (svr & ~0xFF) | LAPIC_SPURIOUS_VECTOR;
    lapic_write(LAPIC_SVR, svr);
    
    lapic_enable();

    lapic_write(LAPIC_TPR, 0);

    lapic_mask_local_sources();

    lapic_timer_init(frequency, wait_func);

    lapic_send_eoi(NULL);

    uint32_t id = lapic_read(LAPIC_ID) >> 24;
    uint32_t version = lapic_read(LAPIC_VER) & 0xFF;

    printk("LAPIC: ID: %d\n", id);
    printk("LAPIC: Version: %d\n", version);

    return 0;
}

const struct irq_controller irq_controller_local_apic = {
    .name = "Local APIC",
    .eoi = lapic_send_eoi,
};
