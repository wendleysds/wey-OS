#include <wey/sched.h>

struct task* current = 0x0;

extern void _switch_to(struct registers* prev, struct registers* to);

void context_switch(struct task* prev, struct task* to){
	if(prev == to || to == current){
		return;
	}

	// TODO: do some checks here, like if the tasks are valid or permissions

	current = to;
	_switch_to(&prev->regs, &to->regs);
}
