#ifndef _X86_PTRACE_H
#define _X86_PTRACE_H

#include <def/compile.h>
#include <stdint.h>

struct registers {
	// pushad regs order
	unsigned long di, si, bp, ksp;
	unsigned long bx, dx, cx, ax;

	// err_code pushed by the processor if
	// theres one
	unsigned long int_no, err_code;

	// Automatically pushed by the processor
	// he dont push SS and SP if the privelege level inst change
	unsigned long ip;
	unsigned short cs, _csh;
	unsigned long flags, sp;
	unsigned short ss, _ssh;
} __packed;

struct task;

static inline unsigned long regs_get_return_value(struct registers *regs){
	return regs->ax;
}

static inline void regs_set_return_value(struct registers *regs, unsigned long value){
	regs->ax = value;
}

static __always_inline int regs_is_user_mode(struct registers *regs){
	return !!(regs->cs & 3);
}

static inline unsigned long regs_get_kernel_stack_pointer(struct registers *regs){
	return regs->sp;
}

static inline unsigned long regs_get_instruction_pointer(struct registers *regs){
	return regs->ip;
}

static inline void regs_set_instruction_pointer(struct registers *regs, unsigned long val){
	regs->ip = val;
}

static inline unsigned long regs_get_frame_pointer(struct registers *regs){
	return regs->bp;
}

static inline unsigned long regs_get_user_stack_pointer(struct registers *regs){
	return regs->sp;
}

static inline void regs_set_user_stack_pointer(struct registers *regs, unsigned long val){
	regs->sp = val;
}

#endif
