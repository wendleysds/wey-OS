#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <arch/i386/idt.h>
#include <io/ports.h>
#include <lib/mem.h>
#include <stdint.h>

/*
 * Simple PS/2 keyboard driver
 */

#define _PS2_COMMAND_PORT 0x64
#define _PS2_INPUT_PORT 0x60
#define _PS2_ENABLE_FIRST_PORT 0xAE

#define _IQR_KEYBOARD_INTERRUPT 0x21
#define _KEYBOARD_KEY_RELEASED 0x80

#define SCANCODE_LSHIFT 0x2A
#define SCANCODE_RSHIFT 0x36

#define SCANCODE_CTRL 0x1D
#define SCANCODE_ALT 0x38

#define SCANCODE_CAPSLOCK 0x3A
#define SCANCODE_NUMLOCK 0x45
#define SCANCODE_SCROLLLOCK 0x46

#define CH_TO_LOWER(c) ((c >= 'A' && c <= 'Z') ? (c + 32) : c)
#define CH_TO_UPPER(c) ((c >= 'a' && c <= 'z') ? (c - 32) : c)

/*QWERT*/

// US
static const char _scancode_us_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
    'z','x','c','v','b','n','m',',','.','/',   0,   '*', 0, ' ',
};

static const char _scancode_us_table_shift[128] = {
	0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
	'\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
	'A','S','D','F','G','H','J','K','L',':','"','~',  0, '|',
	'Z','X','C','V','B','N','M','<','>','?',   0,   '*', 0, ' ',
};

static keyboard_callback_t _kb_callback = 0x0;
static kb_state_t _kb_state;

static char _translate_scancode(uint8_t scancode){
	char key = 0;

	if (scancode < sizeof(_scancode_us_table)) {
		if(_kb_state.shift){
			key = _scancode_us_table_shift[scancode];
		}else{
			key = _scancode_us_table[scancode];
		}

		if(_kb_state.capslock && _kb_state.shift == 0){
			key = CH_TO_UPPER(key);
		}else if(_kb_state.capslock && _kb_state.shift){
			key = CH_TO_LOWER(key);
		}
	}

	return key;
}

static inline void _keyboard_handle_scancode(uint8_t sc){
	uint8_t release = sc & _KEYBOARD_KEY_RELEASED;
	uint8_t code = sc & ~_KEYBOARD_KEY_RELEASED;

	switch (code)
	{
	case SCANCODE_RSHIFT:
	case SCANCODE_LSHIFT:
		_kb_state.shift = !release;
		return;
	case SCANCODE_CTRL:
		_kb_state.ctrl = !release;
		return;
	case SCANCODE_ALT:
		_kb_state.alt = !release;
		return;
	case SCANCODE_CAPSLOCK:
		if(!release)
			_kb_state.capslock = !_kb_state.capslock;
		return;
	case SCANCODE_NUMLOCK:
		if(!release)
			_kb_state.numlock = !_kb_state.numlock;
		return;
	case SCANCODE_SCROLLLOCK:
		if(!release)
			_kb_state.scrolllock = !_kb_state.scrolllock;
		return;
	}

	if(!release){
		char key = _translate_scancode(code);
		if(key && _kb_callback){
			_kb_callback(key);
		}
	}
}

static void _iqr_keyboard_handler(struct InterruptFrame* frame){
	uint8_t scancode = inb(_PS2_INPUT_PORT);
	_keyboard_handle_scancode(scancode);
}

void keyboard_init(){
	outb(_PS2_COMMAND_PORT, _PS2_ENABLE_FIRST_PORT);
	idt_register_callback(_IQR_KEYBOARD_INTERRUPT, _iqr_keyboard_handler);

	_kb_callback = 0x0;
	memset(&_kb_state, 0, sizeof(kb_state_t));
}

void keyboard_set_callback(keyboard_callback_t callback){
	_kb_callback = callback;
}

kb_state_t* keyboard_get_state(){
	return &_kb_state;
}
