#include <wey/process.h>
#include <mm/kheap.h>
#include <lib/string.h>
#include <def/config.h>
#include <def/err.h>
#include <stdint.h>

extern uint16_t next_tid;

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

void task_enqueue(struct TaskQueue* queue, struct Task* task){
    if (!queue || !task){
        return;
    }

    task->snext = NULL;
    task->sprev = queue->tail;

    if (queue->tail) {
        queue->tail->snext = task;
    } else {
        queue->head = task;
    }

    queue->tail = task;
    queue->count++;
}

struct Task* task_dequeue(struct TaskQueue* queue){
    if (!queue || queue->count <= 0) {
        return NULL;
    }

    struct Task* task = queue->head;
	if(!task){
		queue->count = 0;
		return NULL;
	}

    queue->head = task->snext;

    if (queue->head) {
        queue->head->sprev = NULL;
    } else {
        queue->tail = NULL;
    }

    task->snext = NULL;
    task->sprev = NULL;
    queue->count--;
    return task;
}

void task_queue_remove(struct TaskQueue* queue, struct Task* task){
    if (!queue || !task || queue->count <= 0) {
        return;
    }

    if (task->sprev) {
        task->sprev->snext = task->snext;
    } else {
        queue->head = task->snext;
    }

    if (task->snext) {
        task->snext->sprev = task->sprev;
    } else {
        queue->tail = task->sprev;
    }

    task->snext = NULL;
    task->sprev = NULL;
    if (queue->count > 0) {
        queue->count--;
    }
}