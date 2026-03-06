#ifndef _X86_CPU_H
#define _X86_CPU_H

#include <asm/tss.h>

struct task;
struct tss;

struct cpu {
	int id;
	struct tss tss;
	struct task *current;
};

#define cpu_relax() __asm__ volatile("pause" ::: "memory")

void cpu_init();
struct cpu* get_cpu(void);

#endif
