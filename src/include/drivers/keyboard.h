#ifndef _KEYBOARD_H
#define _KEYBOARD_H

typedef struct {
    char shift;
    char ctrl;
    char alt;
    char capslock;
    char numlock;
    char scrolllock;
} kb_state_t;

typedef void(*keyboard_callback_t)(char); 

void keyboard_init();
void keyboard_set_callback(keyboard_callback_t callback);
kb_state_t* keyboard_get_state();

#endif
