#include <core/sched/task.h>
#include <def/config.h>
#include <lib/mem.h>

struct Process* _processes[PROC_MAX];
uint16_t next_tid;

void pid_restart(){
	memset(_processes, 0x0, sizeof(_processes));
	next_tid = 1;
}
