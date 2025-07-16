#ifndef _PROCESS_H
#define _PROCESS_H

#include <core/sched/task.h>
#include <memory/paging.h>
#include <stdint.h>

struct Process {
    uint16_t pid;
    struct PagingDirectory* pageDirectory;
    struct Task* mainTask;

    int argc, envc;
    char** argv;
    char** envp;
} __attribute__((packed));

#endif