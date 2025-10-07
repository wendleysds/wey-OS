#ifndef _PROCESS_H
#define _PROCESS_H

#include <wey/mmu.h>
#include <def/config.h>
#include <stdint.h>

struct Registers {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp;
    uint32_t eip, esp;
    uint32_t ss, cs;
    uint32_t eflags;
} __attribute__((packed));

enum TaskState { 
	TASK_NEW, TASK_READY, TASK_RUNNING, TASK_WAITING, TASK_FINISHED
};

struct Process
{
    uint16_t pid;
    char name[PROC_NAME_MAX];
    int fileDescriptors[PROC_FD_MAX];

    struct mm_struct* mm;

    struct Task *tasks;

    int argc, envc;
    char **argv;
    char **envp;

    char *pwd;
} __attribute__((packed));

struct Task {
    uint16_t tid;
    struct Registers regs;
    struct Process* process;

    void* userStack;
    void* kernelStack;

    enum TaskState state;
    int priority;

    // Keep track on terminate
    struct Task* next;
    struct Task* prev;

    // for scheduler
    struct Task* snext;
    struct Task* sprev;
} __attribute__((packed));

struct TaskQueue{
    struct Task* head;
    struct Task* tail;
    int count;
};

struct Process *process_get(uint16_t pid);
struct Process *process_create(const char *name, const char *pwd, int argc, char **argv, int envc, char **envp);

int process_terminate(struct Process *process);
int process_add_task(struct Process *process, struct Task *task);
int process_remove_task(struct Process *process, struct Task *task);
int process_chdir(struct Process *process, const char *path);

struct Task* task_new(struct Process* proc, void* entry_point);
void task_dispose(struct Task* task);
void task_set_priority(struct Task* task, int priority);
void task_set_state(struct Task* task, enum TaskState state);

void task_enqueue(struct TaskQueue* queue, struct Task* task);
struct Task* task_dequeue(struct TaskQueue* queue);
void task_queue_remove(struct TaskQueue* queue, struct Task* task);

#endif