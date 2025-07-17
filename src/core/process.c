#include <core/process.h>
#include <core/sched/task.h>
#include <def/config.h>
#include <def/status.h>
#include <lib/string.h>
#include <memory/paging.h>
#include <memory/kheap.h>

struct Process* processes[PROC_MAX];
struct Process* current;

struct Process* process_get(uint16_t pid) {
    if (pid >= PROC_MAX || !processes[pid]) {
        return 0x0; // Invalid PID or process not found
    }
    return processes[pid];
}

struct Process* process_create(const char *name, const char *pwd, int argc, char **argv, int envc, char **envp) {
    if (!name || argc < 0 || envc < 0 || argc > PROC_ARG_MAX || envc > PROC_ENV_MAX) {
        return 0x0; // Invalid arguments
    }

    struct Process *process = (struct Process*)kmalloc(sizeof(struct Process));
    if (!process) {
        return 0x0; // Memory allocation failed
    }

    process->pid = 0; // PID will be assigned later
    strncpy(process->name, name, PROC_NAME_MAX - 1);
    process->name[PROC_NAME_MAX - 1] = '\0'; // Ensure null termination

    process->pageDirectory = paging_new_directory(PAGING_TOTAL_ENTRIES_PER_TABLE, FPAGING_P | FPAGING_RW | FPAGING_US);
    if (!process->pageDirectory) {
        kfree(process);
        return 0x0; // Paging directory creation failed
    }

    process->tasks = 0x0;
    process->argc = argc;
    process->envc = envc;

    process->argv = (char**)kmalloc(sizeof(char*) * (argc + 1));
    if (!process->argv) {
        paging_free_directory(process->pageDirectory);
        kfree(process);
        return 0x0; // Memory allocation for argv failed
    }
    
    for (int i = 0; i < argc; i++) {
        process->argv[i] = argv[i];
    }
    process->argv[argc] = 0x0; // Null-terminate the argv array

    process->envp = (char**)kmalloc(sizeof(char*) * (envc + 1));
    if (!process->envp) {
        kfree(process->argv);
        paging_free_directory(process->pageDirectory);
        kfree(process);
        return 0x0; // Memory allocation for envp failed
    }

    for (int i = 0; i < envc; i++) {
        process->envp[i] = envp[i];
    }

    process->envp[envc] = 0x0; // Null-terminate the envp array
    process->pwd = pwd ? strdup(pwd) : strdup("/");
    
    // Assign a PID to the new process
    for (uint16_t pid = 1; pid < PROC_MAX; pid++) {
        if (!processes[pid]) {
            process->pid = pid;
            processes[pid] = process;
            return process; // Successfully created and registered the process
        }
    }

    return 0x0;
}
