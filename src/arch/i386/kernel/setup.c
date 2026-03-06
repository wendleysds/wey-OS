#include <def/init.h>
#include <def/config.h>
#include <asm/cpu.h>
#include <asm/gdt.h>
#include <asm/idt.h>
#include <arch/i386/pic.h>
#include <lib/string.h>

static struct cpu cpus[MAX_CPUS];
static struct gdt_entry gdt[TOTAL_GDT_SEGMENTS];
static struct gdt_descriptor gdt_descriptor;

static inline void gdt_set_tss(int cpu, struct tss *tss){
	gdt[GDT_TSS_BASE_INDEX + cpu] = GDT_ENTRY(
		(unsigned long)&tss, 
		sizeof(struct tss)-1, 
		GDT_TSS_AVAILABLE, 
		GDT_FLAG_32BIT
	);
}

static inline void tss_init(int cpu){
	if(cpu >= MAX_CPUS){
		return;
	}

	struct tss *t = &cpus[cpu].tss;
	memset(t, 0, sizeof(*t));

	t->ss0 = GDT_KERNEL_DATA;
	t->esp0 = 0;
	t->iopb = sizeof(struct tss);

	gdt_set_tss(cpu, t);

	tss_load(GDT_TSS(GDT_TSS_BASE_INDEX + cpu));
}

// no SMP yet, just use cpu 0

struct cpu* get_cpu(void){
	return &cpus[0];
}

void cpu_init(void){
	struct cpu *cpu = &cpus[0];
	cpu->id = 0;
	tss_init(cpu->id);
}

__init void setup_arch(){
	memset(cpus, 0x0, sizeof(cpus));
	for(uint8_t i = 0; i < MAX_CPUS; i++){
		cpus[i].id = -1;
	}

	struct gdt_entry _gdt[TOTAL_GDT_SEGMENTS] = {
		GDT_ENTRY(0, 0, 0, 0),                                    // Null
		GDT_ENTRY(0, 0xFFFFF, GDT_CODE_RING0, GDT_FLAGS_DEFAULT), // Kernel code
		GDT_ENTRY(0, 0xFFFFF, GDT_DATA_RING0, GDT_FLAGS_DEFAULT), // Kernel data
		GDT_ENTRY(0, 0xFFFFF, GDT_CODE_RING3, GDT_FLAGS_DEFAULT), // User code
		GDT_ENTRY(0, 0xFFFFF, GDT_DATA_RING3, GDT_FLAGS_DEFAULT), // User data
	};

	memcpy(gdt, _gdt, sizeof(gdt));

	gdt_descriptor.addr = (uintptr_t)gdt;
	gdt_descriptor.size = sizeof(gdt) - 1;

	gdt_load(&gdt_descriptor);

	cpu_init();

	pic_init(TIMER_FREQUENCY);

	idt_init();
}
