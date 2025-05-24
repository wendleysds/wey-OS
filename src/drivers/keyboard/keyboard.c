#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <arch/i386/idt.h>
#include <io.h>
#include <stdint.h>

/*
 * Simple PS/2 keyboard driver
 */

#define _PS2_PORT 0x64
#define _PS2_KEYBOARD_ENABLE 0xAE

#define _IQR_KEYBOARD_INTERRUPT 0x21

#define _KEYBOARD_INPUT_PORT 0x60
#define _KEYBOARD_CAPSLOCK 0x3A
#define _KEYBOARD_KEY_RELEASED 0x80

static char _scancode_to_ascii[128] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0', '-', '=',
    '\b', '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '[', ']',
    '\n', 0x00, 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'', '`', 
    0x00, '\\', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0', '.',
		'0'
};

static uint8_t capslock = 0;

static char _handle_scancode(uint8_t scanCode){
	if(scanCode > 128){
		return 0x0;
	}

	char c = _scancode_to_ascii[scanCode];
	if(!capslock){
		if(c >= 'A' && c <= 'Z'){
			c += 32;
		}
	}

	return c;
}

static void _iqr_keyboard_handler(struct InterruptFrame* frame){
	uint8_t scanCode = inb(_KEYBOARD_INPUT_PORT);

	if(!(scanCode & _KEYBOARD_KEY_RELEASED)){
		if(scanCode == _KEYBOARD_CAPSLOCK){
			capslock = !capslock;
		}

		char c = _handle_scancode(scanCode);
		if(c != 0){
			terminal_write("%c", c);
		}
	}
	
	outb(0x20, 0x20);
}

void init_keyboard(){
	idt_register_callback(_IQR_KEYBOARD_INTERRUPT, _iqr_keyboard_handler);
	outb(_PS2_PORT, _PS2_KEYBOARD_ENABLE);
}

