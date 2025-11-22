#ifndef _X86_DISPATCHER_H
#define _X86_DISPATCHER_H

struct task;

extern struct task* current;

void context_switch(struct task* prev, struct task* to);

#endif
