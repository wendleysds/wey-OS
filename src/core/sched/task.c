#include <core/sched/task.h>
#include <core/process.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <def/config.h>
#include <def/err.h>
#include <stdint.h>

static uint16_t next_tid = 1;
static inline int alloc_tid() {
    return next_tid++; // Increment and return the next TID
}

struct Task* task_new(struct Process* proc, void* entry_point){
    if(!proc || !entry_point) {
        return ERR_PTR(INVALID_ARG);
    }

    struct Task* task = (struct Task*)kmalloc(sizeof(struct Task));
    if (!task) {
        return ERR_PTR(NO_MEMORY);
    }

    void* userStack = kmalloc(PROC_USER_STACK_SIZE);
    if (!userStack) {
        kfree(task);
        return ERR_PTR(NO_MEMORY);
    }

    void* kernelStack = kmalloc(PROC_KERNEL_STACK_SIZE);
    if (!kernelStack) {
        kfree(userStack);
        kfree(task);
        return ERR_PTR(NO_MEMORY);
    }

    memset(task, 0, sizeof(struct Task));
    memset(userStack, 0, PROC_USER_STACK_SIZE);
    memset(kernelStack, 0, PROC_KERNEL_STACK_SIZE);

    task->tid = alloc_tid();
    task->process = proc;
    task->userStack = userStack;
    task->kernelStack = kernelStack;
    task->state = TASK_NEW;
    task->priority = 0; // Default priority
    task->next = NULL;
    task->prev = NULL;

    // Initialize registers
    task->regs.eip = (uint32_t)entry_point;
    task->regs.esp = PROC_USER_STACK_VIRUTAL_TOP;
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