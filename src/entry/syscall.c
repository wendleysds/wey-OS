#include <syscall.h>
#include <core/kernel.h>
#include <core/process.h>
#include <core/sched.h>
#include <def/status.h>
#include <def/config.h>

extern void _entry_isr80h_32();
extern void _set_idt(uint8_t interrupt_num, void* address);

void syscalls_init(){
    _set_idt(SYSCALL_INTERRUPT_NUM, _entry_isr80h_32);
}

long isr80h_handler(struct InterruptFrame* frame){
    int res = NOT_IMPLEMENTED;
    kernel_page();
    if((res = pcb_save_current(frame)) != SUCCESS){
        return res;
    }

    // TODO: Implement

    pcb_page_current();
    return res;
}