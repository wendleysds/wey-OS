#include <kernel/syscall.h>
#include <kernel/sched.h>
#include <def/errno.h>

SYSCALL_DEFINE0(setsid){
    if (current->pgid == current->pid)
        return -EPERM;

    current->sid = current->pid;
    current->pgid = current->pid;

    return current->sid;
}

SYSCALL_DEFINE1(getsid, pid_t, pid){
    if(pid == 0){
        return current->sid;
    }

    const struct task *task = task_search(pid);
    if(!task){
        return -ESRCH;
    }

    if(task->sid == current->sid){
        return task->pgid;
    }

    return -EPERM;
}

SYSCALL_DEFINE2(setpgid, pid_t, pid, pid_t, pgid){
    struct task *task;
    struct task *group;

    if (pid < 0 || pgid < 0)
        return -EINVAL;

    if (pid == 0)
        pid = current->pid;

    if (pgid == 0)
        pgid = pid;

    if (pid == current->pid) {
        task = current;
    } else {
        task = task_get_child(current, pid);

        if (!task)
            return -ESRCH;
    }

    if (task->sid != current->sid)
        return -EPERM;

    group = task_find_first_with_pgid(pgid);
    if (!group) {
        if (pgid != task->pid)
            return -EPERM;
    } else {
        if (group->sid != current->sid)
            return -EPERM;
    }

    task->pgid = pgid;

    return 0;
}

SYSCALL_DEFINE0(getpgrp){
    return current->pgid;
}
