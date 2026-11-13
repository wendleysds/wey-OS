#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <kernel/signal.h>
#include <kernel/wait.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <fs/fdtable.h>
#include <fs/dcache.h>
#include <def/errno.h>
#include <mm/vma.h>

struct task* init_task;

static LIST_HEAD(tasks_global);
static DEFINE_SPINLOCK(tasks_lock);

void task_global_list_add(struct task *t) {
	spin_lock(&tasks_lock);
	struct task *tmp;
	list_for_each_entry(tmp, &tasks_global, tasks) {
		if (tmp->pid > t->pid) {
			list_add_tail(&t->tasks, &tmp->tasks);
			goto out;
		}
	}
	list_add_tail(&t->tasks, &tasks_global);

out:
	spin_unlock(&tasks_lock);
}

static void task_list_remove(struct task *t) {
	spin_lock(&tasks_lock);
	list_remove(&t->tasks);
	spin_unlock(&tasks_lock);
}

struct task* task_search(pid_t pid) {
    struct task *tmp;
    struct task *found = NULL;

    spin_lock(&tasks_lock);
    
    list_for_each_entry(tmp, &tasks_global, tasks) {
        if (tmp->pid == pid) {
            found = tmp;
            break;
        }

        if (tmp->pid > pid) {
            break;
        }
    }
    
    spin_unlock(&tasks_lock);
    return found;
}

struct task* task_create(const char* name, int priority){ 
	struct task* new_task = kzalloc(sizeof(struct task)); 
	if(likely(new_task)){ 
		strncpy(new_task->name, name, PROC_NAME_MAX); 
		new_task->priority = priority; 
		new_task->state = TASK_NEW;
		INIT_LIST_HEAD(&new_task->tasks); 
		INIT_LIST_HEAD(&new_task->queue); 
		INIT_LIST_HEAD(&new_task->children);
		INIT_LIST_HEAD(&new_task->sibling);
	} 

	return new_task; 
}

static void task_destroy_mm(struct task* task){
	if(!task->mm){
		return;
	}
	
	mm_put(task->mm);
	task->mm = NULL;
}

asmlinkage void task_sleep_uninterruptible(struct task* task){
	task->state = TASK_BLOCKED;
}

asmlinkage void task_sleep_interruptible(struct task* task){
	task->state = TASK_INTERRUPTIBLE;
}

asmlinkage void task_wakeup(struct task* task){
	if (task->state == TASK_INTERRUPTIBLE || task->state == TASK_BLOCKED) {
		task->state = TASK_READY;
		scheduler_add(task);
		return;
	}

	BUG_MSG(task->state >= TASK_ZOMBIE, "Impossible state: %d\n", task->state);
}

int task_default_wakeup(struct wait_queue_entry* entry){
	struct task* task = entry->private;
	if(task->state < TASK_INTERRUPTIBLE){
		return 0;
	}

	task_wakeup(task);
	return 1;
}

void task_reparent_children(struct task* task, struct task* new_parent){
	struct task* child;
	struct task* tmp;

	list_for_each_entry_safe(child, tmp, &task->children, sibling){
		child->parent = new_parent;

		list_remove(&child->sibling);
		list_add(&child->sibling, &new_parent->children);
	}
}

void task_exit(struct task* task, int status){
	if(unlikely(task->pid == 1)){
		panic("Attempting to exit init process! (%d)", status);
	}

	task->exit_code = status;
	task->state = TASK_ZOMBIE;

	task_reparent_children(task, init_task);

	if(task->parent){
		task_wakeup(task->parent);
	}
}

int task_add_file(struct task* task, struct file *file){
	if(!task->files){
		task->files = file_table_create(PROC_FD_TABLE_INITIAL_CAPACITY);
		if(!task->files){
			return -ENOMEM;
		}
	}
	
	return file_table_add_file(task->files, file);
}

int task_remove_file(struct task* task, int fd){
	if (!task || fd < 0) {
        return -EBADF;
    }

    if (!task->files) {
        return -EBADF;
    }

	return file_table_remove_file(task->files, fd);
}

void task_destroy(struct task* task){
	if(task->state != TASK_ZOMBIE && task->state != TASK_NEW){
		panic("Attempting to destroy a non-zombie task (pid: %d, name: %s)", task->pid, task->name);
	}

	if(task->cwd.dentry){
		dentry_put(task->cwd.dentry);
	}

	if(task->kstack){
		kfree(task->kstack);
	}

	if(task->mm){
		task_destroy_mm(task);
	}

	if(task->files){
		file_table_destroy(task->files);
	}

	if(task->pid > 0){
		pid_free(task->pid);
	}

	task_list_remove(task);

	list_remove(&task->sibling);
	kfree(task);
}

void asmlinkage task_handle_prev_status(struct task* prev){
	if(unlikely(prev->pid == 0)){
		return;
	}

	switch (prev->state) {
		case TASK_ZOMBIE:
			if(prev->cwd.dentry){
				dentry_put(prev->cwd.dentry);
				prev->cwd.dentry = NULL;
			}

			if(prev->kstack){
				kfree(prev->kstack);
				prev->kstack = NULL;
			}

			if(prev->mm){
				task_destroy_mm(prev);
				prev->mm = NULL;
			}
			break;
		case TASK_RUNNING:
			prev->state = TASK_READY;
			scheduler_add(prev); 
			return;
		default: return;
	}
}

struct task* task_get_child(struct task* parent, pid_t pid){
	struct task* child;
	list_for_each_entry(child, &parent->children, sibling){
		if(child->pid == pid){
			return child;
		}
	}

	return NULL;
}

struct task* task_find_zombie_child(struct task* parent){
	struct task* child = NULL;

	list_for_each_entry(child, &parent->children, sibling){
		if(child->state == TASK_ZOMBIE){
			return child;
		}
	}

	return NULL;
}

SYSCALL_DEFINE1(exit, int, status){
	task_exit(current, status % 0xFF);
	schedule();
	unreachable();
}

size_t send_signal_to_process_group(pid_t pgid, int sig) {
	size_t count = 0;
	struct task *task;
	spin_lock(&tasks_lock);
	list_for_each_entry(task, &tasks_global, tasks) {
		if (task->pgid == pgid) {
			send_signal(task, sig);
			count++;
		}
	}
	spin_unlock(&tasks_lock);
	return count;
}

struct task *task_find_first_with_pgid(pid_t pgid){
	struct task *task;
	spin_lock(&tasks_lock);
	list_for_each_entry(task, &tasks_global, tasks) {
		if (task->pgid == pgid) {
			spin_unlock(&tasks_lock);
			return task;
		}
	}
	spin_unlock(&tasks_lock);
	return NULL;
}

SYSCALL_DEFINE2(kill, pid_t, pid, int, signum){
	bool sended = false;

    if (pid > 0) {
		struct task* target = task_search(pid);
		if(!target){
			return -ESRCH;
		}

        return send_signal(target, signum);
    }

	if(pid == -1){
		struct task *tmp;
		spin_lock(&tasks_lock);
		list_for_each_entry(tmp, &tasks_global, tasks){
			send_signal(tmp, signum);
			sended = true;
		}
		spin_unlock(&tasks_lock);
	}

	if(pid == 0){
		sended = send_signal_to_process_group(current->pgid, signum) > 0;
	}

	if(pid < -1){
		sended = send_signal_to_process_group(-pid, signum) > 0;
	}
	
	return sended ? 0 : -ESRCH;
}
