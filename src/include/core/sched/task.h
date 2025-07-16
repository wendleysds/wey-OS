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

struct Task {
    uint16_t id;
    struct Registers regs;
    struct Process* process;
    struct Task* next;

    void* userStack;
    uint32_t* kernelStack;
} __attribute__((packed));

#endif