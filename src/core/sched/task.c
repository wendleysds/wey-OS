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
    if(!proc || !entry_point) {
        return NULL; // Invalid arguments
    }

    struct Task* task = (struct Task*)kmalloc(sizeof(struct Task));
    if (!task) {
        return NULL; // Memory allocation failed
    }

    void* userStack = kmalloc(PROC_STACK_SIZE);
    if (!userStack) {
        kfree(task);
        return NULL; // User stack allocation failed
    }

    memset(task, 0, sizeof(struct Task));
    memset(userStack, 0, PROC_STACK_SIZE);
    memset(task->fileDescriptors, -1, sizeof(task->fileDescriptors));

    task->tid = alloc_tid();
    task->process = proc;
    task->userStack = userStack;
    task->kernelStack = NULL;
    task->state = TASK_NEW;
    task->priority = 0; // Default priority
    task->next = NULL;
    task->prev = NULL;

    // Initialize registers
    task->regs.eip = (uint32_t)entry_point;
    task->regs.esp = PROC_VIRTUAL_STACK_START;
    task->regs.ss = USER_DATA_SEGMENT;
    task->regs.cs = USER_CODE_SEGMENT;

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