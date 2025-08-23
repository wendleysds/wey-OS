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
#define _KEYBOARD_BUFFER_SIZE 256

// qwert - US
static const char _scancode_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
    'z','x','c','v','b','n','m',',','.','/',   0,   '*', 0, ' ',
};

static char _kb_buffer[_KEYBOARD_BUFFER_SIZE];
static volatile int _kb_head = 0;
static volatile int _kb_tail = 0;

static int listeners = 0;

static inline void _kb_push(char c) {
	if(listeners <= 0) {
		return;
	}

	int next = (_kb_head + 1) % _KEYBOARD_BUFFER_SIZE;

	if (next != _kb_tail) { // Check if buffer is full
		_kb_buffer[_kb_head] = c;
		_kb_head = next;
	}
}

static inline char _kb_pop() {
	if (_kb_head == _kb_tail) {
		return 0; // Buffer is empty
	}

	char c = _kb_buffer[_kb_tail];
	_kb_tail = (_kb_tail + 1) % _KEYBOARD_BUFFER_SIZE;
	return c;
}

static inline void _kb_clear(char* kbbuffer, int size) {
	for (int i = 0; i < size; i++) {
		kbbuffer[i] = 0;
	}

	_kb_head = 0;
	_kb_tail = 0;
}

static void _iqr_keyboard_handler(struct InterruptFrame* frame){
	uint8_t scancode = inb(_PS2_INPUT_PORT);

	if (scancode & _KEYBOARD_KEY_RELEASED) {
		return;
	}

	if (scancode < sizeof(_scancode_table)) {
		char key = _scancode_table[scancode];
		if (key) {
			_kb_push(key);
		}
	}
}

void keyboard_init(){
	outb(_PS2_COMMAND_PORT, _PS2_ENABLE_FIRST_PORT);
	idt_register_callback(_IQR_KEYBOARD_INTERRUPT, _iqr_keyboard_handler);

	// Clear keyboard buffer
	_kb_clear(_kb_buffer, _KEYBOARD_BUFFER_SIZE);
}

char keyboard_getchar() {
	return _kb_pop();
}

// Ring 0 keyboard read
void keyboard_read(char* buffer, int size) {
	if (!buffer || size <= 0) {
		return; // Invalid arguments
	}

	listeners++;

	int i = 0;
	while (i < size - 1) {
		char c = keyboard_getchar();
		if (c == 0) {
			__asm__ volatile ("hlt"); // Wait for input
			continue;
		}

		terminal_write("%c", c);

		if (c == '\n' || c == '\r') {
			break; // End of input
		}

		buffer[i++] = c;
	}

	buffer[i] = '\0';
	listeners--;
}
