#ifndef _CLIB_SYS_IOCTL_H
#define _CLIB_SYS_IOCTL_H

#include_next <sys/ioctl.h>

int ioctl(int fd, unsigned long op, ...);

#endif /* _CLIB_SYS_IOCTL_H */