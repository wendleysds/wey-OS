#ifndef _CLIB_SYSCALL_H
#define _CLIB_SYSCALL_H

#define SYS_exit 1
#define SYS_read 2
#define SYS_write 3
#define SYS_open 4
#define SYS_close 5
#define SYS_lseek 6
#define SYS_execve 7
#define SYS_fork 8
#define SYS_waitpid 9
#define SYS_getpid 10
#define SYS_brk 11
#define SYS_mmap 12
#define SYS_munmap 13
#define SYS_getdents 14
#define SYS_chdir 15
#define SYS_getcwd 16
#define SYS_unlink 17
#define SYS_mkdir 18
#define SYS_rmdir 19
#define SYS_ioctl 20
#define SYS_reboot 21
#define SYS_clone 22
#define SYS_mount 23
#define SYS_umount 24
#define SYS_posix_spawn 25
#define SYS_dup 26
#define SYS_clock_gettime 27
#define SYS_clock_getres 28
#define SYS_clock_settime 29
#define SYS_nanosleep 30

#define syscall(no, ...) do_syscall(no, ##__VA_ARGS__)

extern long __attribute__((regparm(0))) do_syscall(long no, ...);

#endif