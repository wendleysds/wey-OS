#include <core/process.h>
#include <core/sched/task.h>
#include <def/config.h>
#include <def/status.h>
#include <lib/string.h>
#include <memory/paging.h>
#include <memory/kheap.h>

struct Process* processes[PROC_MAX];
struct Process* current;

static int alloc_pid(){
    for (uint16_t pid = 0; pid < PROC_MAX; pid++) {
        if (!processes[pid]) {
            return pid + 1;
        }
    }
    return -1;
}

struct Process* process_get(uint16_t pid) {
    pid--; // Convert to zero-based index

    if (pid < 0 || pid >= PROC_MAX || !processes[pid]) {
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

    process->pid = alloc_pid();
    if( process->pid < 0) {
        kfree(process);
        return 0x0;
    }

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

    return 0x0;
}

int process_add_task(struct Process *process, struct Task *task){
    if(!process || !task) {
        return INVALID_ARG;
    }

    task->process = process;

    task->next = process->tasks;
    task->prev = NULL;

    if (process->tasks) {
        process->tasks->prev = task;
    }

    process->tasks = task;

    return SUCCESS;
}

int process_remove_task(struct Process *process, struct Task *task){
    if (!process || !task){
        return INVALID_ARG;
    }

    if (task->prev) {
        task->prev->next = task->next;
    } else {
        process->tasks = task->next;
    }

    if (task->next) {
        task->next->prev = task->prev;
    }

    task->next = NULL;
    task->prev = NULL;
    task->process = NULL;

    return SUCCESS;
}

int process_terminate(struct Process *process){
    if (!process) {
        return INVALID_ARG;
    }

    struct Task *task = process->tasks;
    while (task) {
        struct Task *next_task = task->next;
        task_dispose(task);
        task = next_task;
    }

    paging_free_directory(process->pageDirectory);

    kfree(process->argv);
    kfree(process->envp);

    kfree(process);

    processes[process->pid - 1] = NULL; // Mark the process as terminated
    return SUCCESS;
}

int process_chdir(struct Process *process, const char *path){
    if(!process || !path || strlen(path) == 0) {
        return INVALID_ARG;
    }

    char* new_pwd = strdup(path);
    if (!new_pwd) {
        return NO_MEMORY; // Memory allocation failed
    }

    if (process->pwd) {
        kfree((void*)process->pwd); // Free the old pwd
    }

    process->pwd = new_pwd;
    return SUCCESS;
}
