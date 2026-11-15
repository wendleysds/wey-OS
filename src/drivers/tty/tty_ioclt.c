#include <kernel/uaccess.h>
#include <kernel/sched.h>
#include <lib/string.h>
#include <device/tty.h>
#include <def/errno.h>

#include <uapi/sys/ioctls.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

typedef long (*tty_ioctl_fn_t)(struct tty_struct *, unsigned long arg);

static long TTY_IOCTL_TCGETS(struct tty_struct *tty, unsigned long arg){
    if (copy_to_user((void*)arg, &tty->termios, sizeof(struct termios)))
        return -EFAULT;
    return 0;
}

static long TTY_IOCTL_TCSETS(struct tty_struct *tty, unsigned long arg){
    struct termios new_termios;
    if (copy_from_user(&new_termios, (void*)arg, sizeof(struct termios)))
        return -EFAULT;

    tty->termios = new_termios;
    return 0;
}

static long TTY_IOCTL_TCSETSW(struct tty_struct *tty, unsigned long arg){
    return -ENOSYS;
}

static long TTY_IOCTL_TCSETSF(struct tty_struct *tty, unsigned long arg){
    
    return -ENOSYS;
}

static long TTY_IOCTL_TCFLSH(struct tty_struct *tty, unsigned long arg){
    return -ENOSYS;
}

static long TTY_IOCTL_TIOCGWINSZ(struct tty_struct *tty, unsigned long arg){
    if (copy_to_user((void*)arg, &tty->winsize, sizeof(struct winsize)))
        return -EFAULT;
    return 0;
}

static long TTY_IOCTL_TIOCSWINSZ(struct tty_struct *tty, unsigned long arg){
    struct winsize new_winsize;

    if (copy_from_user(&new_winsize, (void *)arg, sizeof(new_winsize)))
        return -EFAULT;

    if (memcmp(&tty->winsize, &new_winsize, sizeof(new_winsize)) == 0)
        return 0;

    tty->winsize = new_winsize;

    if (tty->fg_pgrp > 0)
        send_signal_to_process_group(tty->fg_pgrp, SIGWINCH);

    return 0;
}

static long TTY_IOCTL_TIOCINQ(struct tty_struct *tty, unsigned long arg){
    return -ENOSYS;
}

static long TTY_IOCTL_TIOCOUTQ(struct tty_struct *tty, unsigned long arg){
    return -ENOSYS;
}

static long TTY_IOCTL_TIOCSPGRP(struct tty_struct *tty, unsigned long arg){
    pid_t pgrp;
    if (copy_from_user(&pgrp, (void *)arg, sizeof(pgrp)))
        return -EFAULT;

    if (pgrp <= 0)
        return -EINVAL;

    if (tty->session_sid != current->sid)
        return -ENOTTY;

    struct task *task = task_find_first_with_pgid(pgrp);
    
    if (!task)
        return -ESRCH;
    
    if (task->sid != current->sid)
        return -EPERM;

    tty->fg_pgrp = pgrp;
    return 0;
}

static long TTY_IOCTL_TIOCGPGRP(struct tty_struct *tty, unsigned long arg){
    if (tty->session_sid != current->sid)
        return -ENOTTY;

    if(copy_to_user((void*)arg, &tty->fg_pgrp, sizeof(pid_t)))
        return -EFAULT;
    return 0;
}

static long TTY_IOCTL_TIOCSCTTY(struct tty_struct *tty, unsigned long arg) {
    if (current->sid != current->pid) {
        return -EPERM;
    }

    if (tty->session_sid == current->sid)
        return 0;

    if (tty_find_with_session(current->sid, tty)) {
        return -EINVAL;
    }

    if (tty->session_sid != 0) {
        if (arg != 1)
            return -EPERM;

        // TODO: support caps and check CAP_SYS_ADMIN.
        return -EPERM;
    }

    // TODO: check if current has another controlling tty.

    tty->session_sid = current->sid;
    tty->fg_pgrp = current->pgid; 
    return 0;
}

static long TTY_IOCTL_TIOCGSID(struct tty_struct *tty, unsigned long arg){
    if (tty->session_sid == 0)
        return -ENOTTY;
    
    if (copy_to_user((void*)arg, &tty->session_sid, sizeof(pid_t)))
        return -EFAULT;
    return 0;
}

const tty_ioctl_fn_t tty_ioctl_table[] = {
    [_IO_NR(TCGETS)] = TTY_IOCTL_TCGETS,
    [_IO_NR(TCSETS)] = TTY_IOCTL_TCSETS,
    [_IO_NR(TCSETSW)] = TTY_IOCTL_TCSETSW,
    [_IO_NR(TCSETSF)] = TTY_IOCTL_TCSETSF,
    [_IO_NR(TCFLSH)] = TTY_IOCTL_TCFLSH,
    [_IO_NR(TIOCGWINSZ)] = TTY_IOCTL_TIOCGWINSZ,
    [_IO_NR(TIOCSWINSZ)] = TTY_IOCTL_TIOCSWINSZ,
    [_IO_NR(TIOCINQ)] = TTY_IOCTL_TIOCINQ,
    [_IO_NR(TIOCOUTQ)] = TTY_IOCTL_TIOCOUTQ,
    [_IO_NR(TIOCSPGRP)] = TTY_IOCTL_TIOCSPGRP,
    [_IO_NR(TIOCGPGRP)] = TTY_IOCTL_TIOCGPGRP,
    [_IO_NR(TIOCSCTTY)] = TTY_IOCTL_TIOCSCTTY,
    [_IO_NR(TIOCGSID)] = TTY_IOCTL_TIOCGSID,
};

const size_t tty_ioctl_table_count = ARRAY_SIZE(tty_ioctl_table);

