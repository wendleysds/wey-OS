#ifndef _PROCESS_H
#define _PROCESS_H

#include <core/sched/task.h>
#include <memory/paging.h>
#include <def/config.h>
#include <stdint.h>

struct Process
{
    uint16_t pid;
    char name[PROC_NAME_MAX];

    struct PagingDirectory *pageDirectory;
    struct Task *tasks;

    int argc, envc;
    char **argv;
    char **envp;

    const char *pwd;
} __attribute__((packed));

struct Process *process_get(uint16_t pid);
struct Process *process_create(const char *name, const char *pwd, int argc, char **argv, int envc, char **envp);
int process_terminate(struct Process *process);
struct Process* process_current();
int process_add_task(struct Process *process, struct Task *task);
int process_remove_task(struct Process *process, struct Task *task);
int process_switch(struct Process *process);
int process_chdir(struct Process *process, const char *path);

#endif