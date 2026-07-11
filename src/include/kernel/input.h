#ifndef _INPUT_H
#define _INPUT_H

#include <kernel/input/event.h>

void input_report(const struct input_event *event);

#endif