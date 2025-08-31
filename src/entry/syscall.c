#include <syscall.h>
#include <core/kernel.h>
#include <core/process.h>
#include <core/sched.h>
#include <def/err.h>
#include <def/config.h>
#include <lib/mem.h>
#include <drivers/terminal.h>
#include <uaccess.h>

extern void _entry_isr80h_32();
extern void _set_idt(uint8_t interrupt_num, void* address, uint8_t flags);

static sys_fn_t _syscall_table[SYSCALLS_MAX];

SYSCALL_DEFINE2(write_console, const void*, buffer, size_t, size){
	char kbuffer[256];
	int res = copy_from_user(kbuffer, buffer, size);

	if(IS_STAT_ERR(res)){
		return res;
	}

	for(size_t i = 0; i < size; i++){
		terminal_write("%c", kbuffer[i]);
	}

	return size;
}

void syscalls_init(){
	_set_idt(SYSCALL_INTERRUPT_NUM, _entry_isr80h_32, IDT_PRESENT | IDT_DPL3 | IDT_TYPE_INT_GATE32);
	memset(_syscall_table, 0x0, sizeof(_syscall_table));

	syscalls_register(1, (sys_fn_t)sys_write_console);
}

static inline long _dispatch_syscall(long sys_no, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6){
	if (sys_no < 0 || sys_no >= SYSCALLS_MAX || !_syscall_table[sys_no]){		
		return INVALID_ARG;
	}

    return _syscall_table[sys_no](arg1, arg2, arg3, arg4, arg5, arg6);
}

long isr80h_handler(struct InterruptFrame* frame){
	int res = _dispatch_syscall(
		frame->eax,
		frame->ebx,
		frame->ecx,
		frame->edx,
		frame->esi,
		frame->edi,
		frame->ebp
	);

	pcb_page_current();
	return res;
}

int syscalls_register(long sys_no, sys_fn_t syscall_fn){
	if (sys_no < 0 || sys_no >= SYSCALLS_MAX || _syscall_table[sys_no]){		
		return INVALID_ARG;
	}

	_syscall_table[sys_no] = syscall_fn;

	return SUCCESS;
}
