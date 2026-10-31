#ifndef _X86_CPU_H
#define _X86_CPU_H

#include <def/compile.h>
#include <asm/tss.h>
#include <stdbool.h>

struct task;
struct tss;

// The necessary flags to make this CPU actually work
struct cpu_caps {
	bool has_tsc;
	bool has_msr;
	bool has_apic;
	bool has_nx;
	bool has_pae;
	bool has_pse;
	bool has_pge;
	bool has_fpu;
	bool has_mmx;
	bool has_avx;
	bool has_sse;
	bool has_xsave;
	bool has_osxsave;
	bool has_avx_cpu;
};

struct cpu {
	int id;
	struct tss tss;
	struct task *current;
struct cpu_caps caps;
};

static inline void cpu_relax(void) {
	__asm__ volatile("pause" ::: "memory");
}

void cpu_init(struct cpu *cpu);
void cpu_print_info(struct cpu *cpu);
void cpu_detect_caps(struct cpu *cpu);
struct cpu *get_cpu(void);

#endif
