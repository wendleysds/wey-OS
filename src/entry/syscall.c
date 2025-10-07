#include <wey/syscall.h>
#include <wey/panic.h>
#include <wey/sched.h>
#include <def/err.h>
#include <def/config.h>
#include <lib/string.h>

#define ll long long

#define __SYSCALL(no, name) \
	case no:\
		extern long __se_##name(ll, ll, ll, ll, ll, ll); \
		return (sys_fn_t) __se_##name; \

extern void _entry_isr80h_32();
extern void _set_idt(uint8_t interrupt_num, void* address, uint8_t flags);

extern void kernel_registers();
extern void user_registers();

static long _invsys(ll, ll, ll, ll, ll, ll){
	return INVALID_ARG;
}

static inline long _dispatch_syscall(long sys_no, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6){
	if (sys_no < 0){		
		return INVALID_ARG;
	}

    return _syscall(sys_no)(arg1, arg2, arg3, arg4, arg5, arg6);
}

void syscalls_init(){
	_set_idt(SYSCALL_INTERRUPT_NUM, _entry_isr80h_32, IDT_PRESENT | IDT_DPL3 | IDT_TYPE_INT_GATE32);
}

long isr80h_handler(struct InterruptFrame* frame){
	kernel_registers();

	int res = _dispatch_syscall(
		frame->eax,
		frame->ebx,
		frame->ecx,
		frame->edx,
		frame->esi,
		frame->edi,
		frame->ebp
	);

	user_registers();
	return res;
}

sys_fn_t _syscall(long no){
	switch (no)
	{
	#include "syscalls/syscalltbl.h"
	default:
		return (sys_fn_t) _invsys;
	}
}