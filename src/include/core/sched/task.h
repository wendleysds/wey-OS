#ifndef _TASK_H
#define _TASK_H

#include <core/process.h>
#include <stdint.h>

struct Registers {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp;
    uint32_t eip, esp;
    uint32_t ss, cs;
    uint32_t eflags;
} __attribute__((packed));

enum TaskState { 
    TASK_NEW, TASK_RUNNING, TASK_READY, TASK_WAITING, TASK_FINISHED
};

struct Task {
    uint16_t tid;
    struct Registers regs;
    struct Process* process;

    void* userStack;
    uint32_t* kernelStack;

    enum TaskState state;

    int priority;

    struct Task* next;
    struct Task* prev;
} __attribute__((packed));

struct Task* task_new(struct Process* proc, void* entry_point);
void task_dispose(struct Task* task);
void task_switch(struct Task* current, struct Task* next);
void task_set_priority(struct Task* task, int priority);
void task_set_state(struct Task* task, enum TaskState state);

#endif