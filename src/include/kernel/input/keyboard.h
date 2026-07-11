#ifndef _INPUT_KEYBOARD_H
#define _INPUT_KEYBOARD_H

typedef struct {
	char shift;
	char ctrl;
	char alt;
	char capslock;
	char numlock;
	char scrolllock;
} kb_state_t;

#endif