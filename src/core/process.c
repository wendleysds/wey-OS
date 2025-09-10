#include <core/process.h>
#include <core/sched.h>
#include <core/sched/task.h>
#include <def/config.h>
#include <def/err.h>
#include <lib/string.h>
#include <lib/mem.h>
#include <memory/kheap.h>
#include <mmu.h>

static struct Process* _processes[PROC_MAX];

static int alloc_pid(){
    for (uint16_t pid = 0; pid < PROC_MAX; pid++) {
        if (!_processes[pid]) {
            return pid + 1;
        }
    }
    return -1;
}

struct Process* process_get(uint16_t pid) {
    pid--; // Convert to zero-based index

    if (pid < 0 || pid >= PROC_MAX || !_processes[pid]) {
        return 0x0; // Invalid PID or process not found
    }
    
    return _processes[pid];
}

struct Process* process_create(const char *name, const char *pwd, int argc, char **argv, int envc, char **envp) {
    if (!name || argc < 0 || envc < 0 || argc > PROC_ARG_MAX || envc > PROC_ARG_MAX) {
        return 0x0;
    }

    struct Process *process = (struct Process*)kmalloc(sizeof(struct Process));
    if (!process) {
        return ERR_PTR(NO_MEMORY);
    }

    process->pid = alloc_pid();
    if (process->pid < 0) {
        kfree(process);
        return ERR_PTR(OUT_OF_BOUNDS);
    }

    strncpy(process->name, name, PROC_NAME_MAX - 1);
    process->name[PROC_NAME_MAX - 1] = '\0';

	process->mm = (struct mm_struct*)kzalloc(sizeof(struct mm_struct));
	if(!process->mm){
		kfree(process);
        return ERR_PTR(NO_MEMORY);
	}

    process->mm->pageDirectory = mmu_create_page();
    if (!process->mm->pageDirectory) {
        kfree(process);
		kfree(process->mm);
        return ERR_PTR(NO_MEMORY);
    }

	memset(process->fileDescriptors, 0, sizeof(process->fileDescriptors));

    process->tasks = NULL;
    process->pwd = NULL;

    process->argc = argc;
    process->argv = NULL;

    process->envc = envc;
    process->envp = NULL;

    // Allocate and copy argv
    if (argc > 0) {
        process->argv = (char**)kmalloc(sizeof(char*) * (argc + 1));
        if (!process->argv) {
            goto fail;
        }

        for (int i = 0; i < argc; i++) {
            process->argv[i] = strdup(argv[i]);
            if (!process->argv[i]) {
                for (int j = 0; j < i; j++) kfree(process->argv[j]);
                kfree(process->argv);
                process->argv = NULL;
                goto fail;
            }
        }

        process->argv[argc] = NULL;
    }

    // Allocate and copy envp
    if (envc > 0) {
        process->envp = (char**)kmalloc(sizeof(char*) * (envc + 1));
        if (!process->envp) {
            goto fail;
        }

        for (int i = 0; i < envc; i++) {
            process->envp[i] = strdup(envp[i]);
            if (!process->envp[i]) {
                for (int j = 0; j < i; j++) kfree(process->envp[j]);
                kfree(process->envp);
                process->envp = NULL;
                goto fail;
            }
        }
        process->envp[envc] = NULL;
    }

    // Set working directory
    process->pwd = pwd ? strdup(pwd) : strdup("/");
    if (!process->pwd){
        goto fail;
    }

    _processes[process->pid - 1] = process;
    return process;

fail:
    if (process->argv) {
        for (int i = 0; i < argc; i++) {
            if (process->argv[i]) kfree(process->argv[i]);
        }

        kfree(process->argv);
    }
    if (process->envp) {
        for (int i = 0; i < envc; i++) {
            if (process->envp[i]) kfree(process->envp[i]);
        }

        kfree(process->envp);
    }
    
    if (process->pwd){
        kfree(process->pwd);
    }

    if (process->mm){
		if(process->mm->pageDirectory){
        	mmu_destroy_page(process->mm->pageDirectory);
		}

		kfree(process->mm);
    }

    kfree(process);
    return ERR_PTR(NO_MEMORY);
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

    return NOT_IMPLEMENTED;
}

int process_remove_task(struct Process *process, struct Task *task){
    if (!process || !task){
        return INVALID_ARG;
    }

    if(task->process != process){
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

    scheduler_remove_task(task);

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

        scheduler_remove_task(task);

        task_dispose(task);
        task = next_task;
    }

	vma_destroy(process->mm);

    kfree(process->argv);
    kfree(process->envp);

    kfree(process);

    _processes[process->pid - 1] = NULL; // Mark the process as terminated
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
