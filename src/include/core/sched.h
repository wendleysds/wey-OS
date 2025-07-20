#ifndef _CPU_SCHEDULER_H
#define _CPU_SCHEDULER_H

#include <core/sched/task.h>
#include <stdint.h>

void schedule_add_task(struct Task* task);
struct Task* scheduler_pick_next();

void schedule();

#endif
