#include <core/sched/task.h>
#include <core/process.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <def/config.h>

#include <stdint.h>

static uint16_t next_tid = 1;
static int alloc_tid() {
    return next_tid++; // Increment and return the next TID
}

struct Task* task_new(struct Process* proc, void* entry_point){
    struct Task* task = (struct Task*)kmalloc(sizeof(struct Task));
    if (!task) {
        return NULL; // Memory allocation failed
    }

    task->tid = alloc_tid();
    task->process = proc;
    task->userStack = NULL;
    task->kernelStack = NULL;
    task->state = TASK_READY;
    task->priority = 0; // Default priority
    task->next = NULL;

    memset(&task->regs, 0x0, sizeof(struct Registers));
    
    task->regs.eip = (uint32_t)entry_point;

    process_add_task(proc, task);

    return task;
}

void task_dispose(struct Task* task){
    if (!task) {
        return;
    }

    if(task->next || task->prev) {
        struct Process* proc = task->process;
        process_remove_task(proc, task);
    }

    if (task->userStack) {
        kfree(task->userStack);
    }

    if (task->kernelStack) {
        kfree(task->kernelStack);
    }

    next_tid--;

    kfree(task);
}

void task_set_state(struct Task* task, enum TaskState state){
    if(state < TASK_RUNNING || state > TASK_FINISHED) {
        return; // Invalid state
    }

    task->state = state;
}