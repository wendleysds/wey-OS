#include <kernel/printk.h>
#include <def/config.h>
#include <lib/serial.h>
#include <stddef.h>

static printk_echo_function _printk_ech = NULL;
static char printk_circular_buffer[PRINTK_BUFFER_SIZE];

static uint64_t printk_cursor = 0;

// Not SMP-safe, but we don't have SMP yet.
static inline void printk_write_char(char c){
	printk_circular_buffer[printk_cursor % PRINTK_BUFFER_SIZE] = c;
	printk_cursor++;
}

void printk_show_buffer(){
	if(!_printk_ech)
		return;

	if(printk_cursor == 0)
		return;

	uint64_t start = 0; 
	if(printk_cursor > PRINTK_BUFFER_SIZE){ 
		start = printk_cursor - PRINTK_BUFFER_SIZE; 
	}

	uint64_t start_idx = start % PRINTK_BUFFER_SIZE;
	uint64_t end_idx = printk_cursor % PRINTK_BUFFER_SIZE;
	if(start_idx < end_idx){
		_printk_ech(&printk_circular_buffer[start_idx], end_idx - start_idx);
	}else{
		_printk_ech(&printk_circular_buffer[start_idx], PRINTK_BUFFER_SIZE - start_idx);
		_printk_ech(&printk_circular_buffer[0], end_idx);
	}
}

void printk_set_echo(printk_echo_function f){
	_printk_ech = f;
}

int printk(const char* restrict fmt, ...){
	char buffer[1024];

	va_list args;
	va_start(args, fmt);
	int c = vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (_printk_ech)
		_printk_ech(buffer, c);

	for(int i = 0; buffer[i] != '\0' || i > 1024; i++){
		printk_write_char(buffer[i]);
		serial_putchar(buffer[i]);
	}

	return c;
}