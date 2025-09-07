#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_write 100

extern long do_syscall(long no, long arg1, long arg2, long arg3, long arg4);

static inline long syscall(long no, long arg1, long arg2, long arg3, long arg4) {
	return do_syscall(no, arg1, arg2, arg3, arg4);
}

#endif