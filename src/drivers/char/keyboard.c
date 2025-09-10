#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <arch/i386/idt.h>
#include <io/ports.h>
#include <stdint.h>

/*
 * Simple PS/2 keyboard driver
 */

#define _PS2_COMMAND_PORT 0x64
#define _PS2_INPUT_PORT 0x60
#define _PS2_ENABLE_FIRST_PORT 0xAE

#define _IQR_KEYBOARD_INTERRUPT 0x21
#define _KEYBOARD_KEY_RELEASED 0x80

/*QWERT*/

// US
static const char _scancode_us_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
    'z','x','c','v','b','n','m',',','.','/',   0,   '*', 0, ' ',
};

/*static const char _scancode_us_table_shift[128] = {
	0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
	'\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
	'A','S','D','F','G','H','J','K','L',':','"','~',  0, '|',
	'Z','X','C','V','B','N','M','<','>','?',   0,   '*', 0, ' ',
};*/

static keyboard_callback_t _kb_callback = 0x0;

/*static char shifted = 0;
static char capslock = 0;*/

static void _iqr_keyboard_handler(struct InterruptFrame* frame){
	uint8_t scancode = inb(_PS2_INPUT_PORT);

	if (scancode & _KEYBOARD_KEY_RELEASED) {
		return;
	}

	if (scancode < sizeof(_scancode_us_table)) {
		char key = _scancode_us_table[scancode];
		if (key) {
			if(_kb_callback){
				_kb_callback(key);
			}	
		}
	}
}

void keyboard_init(){
	outb(_PS2_COMMAND_PORT, _PS2_ENABLE_FIRST_PORT);
	idt_register_callback(_IQR_KEYBOARD_INTERRUPT, _iqr_keyboard_handler);
	_kb_callback = 0x0;
}

void keyboard_set_callback(keyboard_callback_t callback){
	_kb_callback = callback;
}
