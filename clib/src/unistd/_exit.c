#include <unistd.h>
#include <syscall.h>

void __attribute__((noreturn)) _exit(int status){
    syscall(SYS_exit, status);
    asm volatile("cli; hlt"); // force GPF
    while(1);
}