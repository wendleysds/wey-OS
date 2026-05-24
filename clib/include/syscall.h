#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_write 100

extern long __attribute__((regparm(0))) do_syscall(long no, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

static inline long __attribute__((regparm(0))) syscall(long no, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
	return do_syscall(no, arg1, arg2, arg3, arg4, arg5, arg6);
}

#endif