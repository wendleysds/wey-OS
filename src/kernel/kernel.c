#include "kernel.h"
#include "../terminal/terminal.h"

void init_kernel(){
    terminal_write("\nInitializing kernel...\n", 0x0F);
}

void panic(const char* msg){
    terminal_write(msg, TERMINAL_DEFAULT_COLOR);
    while (1);
}

void kernel_page(){

}
void kernel_registers(){

}