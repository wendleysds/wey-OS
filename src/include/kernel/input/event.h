#ifndef _INPUT_EVENT_H
#define _INPUT_EVENT_H

#include <stdint.h>
#include <lib/list.h>
#include "keycodes.h"

enum input_event_type {
	INPUT_EVENT_KEY,
	INPUT_EVENT_MOUSE,
};

enum input_key_state {
	INPUT_KEY_RELEASED,
	INPUT_KEY_PRESSED,
};

enum input_modifier {
	INPUT_MOD_SHIFT      = 1 << 0,
	INPUT_MOD_CTRL       = 1 << 1,
	INPUT_MOD_ALT        = 1 << 2,
	INPUT_MOD_CAPSLOCK   = 1 << 3,
	INPUT_MOD_NUMLOCK    = 1 << 4,
	INPUT_MOD_SCROLLLOCK = 1 << 5,
};

struct input_key_event {
	enum input_keycode keycode;
	enum input_key_state state;
	uint32_t modifiers;
};

struct input_mouse_event {
	int dx;
	int dy;
	uint32_t buttons;
};

struct input_event {
	enum input_event_type type;

	union {
		struct input_key_event key;
		struct input_mouse_event mouse;
	};
};

struct input_handler {
	void (*event)(struct input_handler *handler, const struct input_event *event);
	void *private;
	struct list_head list;
};

int input_register_handler(struct input_handler *handler);
void input_unregister_handler(struct input_handler *handler);

#endif