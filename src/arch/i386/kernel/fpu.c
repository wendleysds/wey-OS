#include <kernel/sched.h>
#include <kernel/interrupt.h>
#include <kernel/signal.h>
#include <kernel/panic.h>
#include <kernel/init.h>
#include <def/errno.h>
#include <asm/cpu.h>
#include <asm/idt.h>
#include <asm/gdt.h>
#include <asm/fpu.h>
#include <asm/ptrace.h>
#include <asm/cpuflags.h>

static inline unsigned long read_cr0(void){
	unsigned long v;
	__asm__ volatile("mov %%cr0, %0" : "=r"(v));
	return v;
}

static inline void write_cr0(unsigned long v){
	__asm__ volatile("mov %0, %%cr0" :: "r"(v) : "memory");
}

static inline void clts(void){ 
	__asm__ volatile("clts");
}

static inline void stts(void){ 
	write_cr0(read_cr0() | X86_CR0_TS);
}

static inline void fpu_save(struct fpu_state *s){
	__asm__ volatile("fnsave %0" : "=m"(*s) :: "memory");
}

static inline void fpu_restore(const struct fpu_state *s){
	__asm__ volatile("frstor %0" :: "m"(*s) : "memory");
}

void fpu_nm_handler(struct irq_info *info){
	struct cpu *cpu = get_cpu();
	struct task *t = current;
	struct registers *regs = info->cpu.regs;

	if (cpu->fpu_owner && cpu->fpu_owner->state == TASK_ZOMBIE)
		panic("fpu_owner aponta para task zumbi");

	clts();

	if (!t || !regs_is_user_mode(regs))
		panic("FPU used in kernel mode (ip=%#lx)", regs->ip);

	if (cpu->fpu_owner == t)
		return;

	if (cpu->fpu_owner)
		fpu_save(&cpu->fpu_owner->fpu);

	if (t->fpu_used) {
		fpu_restore(&t->fpu);
	} else {
		__asm__ volatile("fninit");
		t->fpu_used = true;
	}

	cpu->fpu_owner = t;
}

void fpu_switch_to(struct task *to){
	if (get_cpu()->fpu_owner == to) clts();
	else                            stts();
}

void fpu_flush(struct task *t){
	struct cpu *cpu = get_cpu();
	if (cpu->fpu_owner != t) return;

	clts();
	fpu_save(&t->fpu);
	cpu->fpu_owner = NULL;
	stts();
}

void fpu_copy(struct task *dst, struct task *src){
	fpu_flush(src);
	dst->fpu = src->fpu;
	dst->fpu_used = src->fpu_used;
}

void fpu_set_state(struct task *t, const struct fpu_state *s, bool used){
	fpu_reset(t);
	if (used)
		t->fpu = *s;
	t->fpu_used = used;
}

void fpu_reset(struct task *t){
	struct cpu *cpu = get_cpu();
	t->fpu_used = false;
	if (cpu->fpu_owner == t) {
		cpu->fpu_owner = NULL;
		stts();
	}
}

void fpu_release(struct task *t){
	struct cpu *cpu = get_cpu();
	if (cpu->fpu_owner == t)
		cpu->fpu_owner = NULL;
}

static void fpu_mf_handler(struct irq_info *info){
	clts();
	__asm__ volatile("fnclex");

	if (!info->cpu.from_user)
		panic("x87 exception in kernel");

	send_signal(current, SIGFPE);
}

int __init fpu_init(void){
	struct cpu *cpu = get_cpu();
	if (!cpu->caps.has_fpu)
		return -ENODEV;

	unsigned long cr0 = read_cr0();
	cr0 &= ~(X86_CR0_EM | X86_CR0_TS);
	cr0 |= X86_CR0_MP | X86_CR0_NE;
	write_cr0(cr0);

	unsigned short sw = 0xFFFF;
	__asm__ volatile("fninit; fnstsw %0" : "+m"(sw));
	if (sw != 0)
		return -ENODEV;

	cpu->fpu_owner = NULL;

	raw_interrupt_register(7, fpu_nm_handler, NULL, 0);

	raw_interrupt_register(16, fpu_mf_handler, NULL, 0);

	stts();
	return 0;
}

arch_initcall(fpu_init);