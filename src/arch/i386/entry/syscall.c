#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <asm/idt.h>
#include <def/errno.h>

#define ARG(x) long arg##x

#define __SYSCALL(no, name) \
	case no:\
		extern asmlinkage long __se_##name(long, long, long, long, long, long); \
		return (sys_fn_t) __se_##name; \

extern void _entry_isr80h_32();
extern void _set_idt(uint8_t interrupt_num, void* address, uint8_t flags);

extern void kernel_registers();
extern void user_registers();

static long _invsys(long a, ...){
	return -ENOENT;
}

static inline sys_fn_t syscall(long no){
	switch (no)
	{
	#include "syscalls/syscalltbl.h"
	default:
		return (sys_fn_t) _invsys;
	}
}

static inline long dispatch_syscall(long sys_no, ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6)){
    return syscall(sys_no)(arg1, arg2, arg3, arg4, arg5, arg6);
}

void __init syscalls_init(){
	idt_set_gate(
		SYSCALL_INTERRUPT_NUM,
		(uint32_t)_entry_isr80h_32,
		GDT_KERNEL_CODE,
		(IDT_PRESENT | IDT_DPL3 | IDT_TYPE_INT_GATE32)
	);
}

void isr80h_handler(struct registers* regs){
	kernel_registers();

	current->regs = *regs;

	int res = dispatch_syscall(
		regs->ax,
		regs->bx,
		regs->cx,
		regs->dx,
		regs->si,
		regs->di,
		regs->bp
	);

	*regs = current->regs;
	regs->ax = res;

	user_registers();
}
