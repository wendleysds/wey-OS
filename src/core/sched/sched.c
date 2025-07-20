#include <core/sched.h>
#include <core/sched/task.h>
#include <memory/kheap.h>
#include <stdint.h>

static struct Task* _queueHead = 0x0;
static struct Task* _queueTail = 0x0;

void schedule_add_task(struct Task* task){
    task->snext = 0x0;
    task->sprev = 0x0;

    if (_queueTail == 0x0) {
        _queueHead = task;
        _queueTail = task;
    } else {
        _queueTail->snext = task;
        task->sprev = _queueTail;
        _queueTail = task;
    }

    task->state = TASK_READY;
}

struct Task* scheduler_pick_next(){
    if (_queueHead == 0x0) {
        return 0x0;
    }

    struct Task* next_task = _queueHead;

    _queueHead = _queueHead->snext;

    if (_queueHead != 0x0) {
        _queueHead->sprev = 0x0;
    } else {
        _queueTail = 0x0;
    }

    next_task->snext = 0x0;
    next_task->sprev = 0x0;

    return next_task;
}

void schedule();
