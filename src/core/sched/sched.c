#include <core/sched.h>
#include <core/sched/task.h>
#include <core/kernel.h>
#include <memory/kheap.h>
#include <def/status.h>
#include <stdint.h>
#include <drivers/terminal.h>
#include <io/ports.h>

struct TaskQueue{
    struct Task* head;
    struct Task* tail;
    int count;
};

static struct TaskQueue _readyQueue = { NULL, NULL, 0 };
static struct TaskQueue _waitQueue = { NULL, NULL, 0 };
static struct TaskQueue _terminateQueue = { NULL, NULL, 0 };

static uint64_t ticks = 0;

static struct Task _idleTask;

static void _idle_task_entry(){
    terminal_write("\nHello From idle Task!\n");
    while (1) {
        __asm__ volatile ("hlt");
    }
    
}

static void init_task_idle(){
    memset(&_idleTask, 0x0, sizeof(_idleTask));

    _idleTask.tid = 0;
    _idleTask.state = TASK_READY;
    _idleTask.priority = 0;
    _idleTask.process = NULL;

    _idleTask.kernelStack = kmalloc(PROC_KERNEL_STACK_SIZE);
    _idleTask.userStack = NULL;

    memset(_idleTask.fileDescriptors, 0, sizeof(_idleTask.fileDescriptors));

    _idleTask.regs.eip = (uintptr_t)_idle_task_entry;
    _idleTask.regs.esp = (uintptr_t)_idleTask.kernelStack + PROC_KERNEL_STACK_SIZE;
    _idleTask.regs.ebp = _idleTask.regs.esp;
    _idleTask.regs.cs = KERNEL_CODE_SELECTOR;
}

static void _task_enqueue(struct TaskQueue* queue, struct Task* task){
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

static struct Task* _task_dequeue(struct TaskQueue* queue){
    if (!queue || queue->count <= 0) {
        return NULL;
    }

    struct Task* task = queue->head;
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

static void _task_queue_remove(struct TaskQueue* queue, struct Task* task){
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

static void _schedule_iqr_PIT_handler(struct InterruptFrame* frame){
    if(pcb_save_current(frame) == NULL_PTR){
        panic("pcb_save_current(*frame): Invalid frame pointer!");
    }

    outb(0x20, 0x20);
    schedule();
}

void schedule(){
    struct Task* next = scheduler_pick_next();
    if(!next){
        next = &_idleTask;
    }

    next->state = TASK_RUNNING;
    if(dispatcher_load(next) != SUCCESS){
        panic("dispatcher_load(): Invalid task!");
    }
}

void scheduler_init(){
    init_task_idle();
    idt_register_callback(0x20, _schedule_iqr_PIT_handler);
    ticks = 0;
}

void sheduler_enqueue_auto(struct Task* task){
    switch (task->state) {
        case TASK_RUNNING:
        case TASK_READY:
            task->state = TASK_READY;
            _task_enqueue(&_readyQueue, task);
            break;

        case TASK_WAITING:
            _task_enqueue(&_waitQueue, task);
            break;

        case TASK_FINISHED:
            _task_enqueue(&_terminateQueue, task);
            break;

        default:
            panic("sheduler_enqueue_auto(): Unknown task state!");
    }
}

void scheduler_add_task(struct Task* task){
    task->state = TASK_READY;
    _task_enqueue(&_readyQueue, task);
}

void scheduler_remove_task(struct Task* task){
    struct TaskQueue* queue;
    switch (task->state)
    {
    case TASK_READY:
        queue = &_readyQueue;
        break;
    case TASK_WAITING:
        queue = &_waitQueue;
        break;
    case TASK_FINISHED:
        queue = &_terminateQueue;
        break;
    default:
        return;
    }

    _task_queue_remove(queue, task);
}

struct Task* scheduler_pick_next(){
    return _task_dequeue(&_readyQueue);
}
