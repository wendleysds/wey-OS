#ifndef _UAPI_SYS_IOCTLS_H
#define _UAPI_SYS_IOCTLS_H

#include "ioctl.h"

#define TTY_IOC_MAGIC 'T'

#define TCGETS  _IOR(TTY_IOC_MAGIC, 0x01, struct termios)
#define TCSETS  _IOW(TTY_IOC_MAGIC, 0x02, struct termios)
#define TCSETSW _IOW(TTY_IOC_MAGIC, 0x03, struct termios)
#define TCSETSF _IOW(TTY_IOC_MAGIC, 0x04, struct termios)
#define TCFLSH _IOW(TTY_IOC_MAGIC, 0x05, int)
#define TIOCGWINSZ _IOR(TTY_IOC_MAGIC, 0x10, struct winsize)
#define TIOCSWINSZ _IOW(TTY_IOC_MAGIC, 0x11, struct winsize)
#define TIOCINQ _IOR(TTY_IOC_MAGIC, 0x12, int)
#define TIOCOUTQ _IOR(TTY_IOC_MAGIC, 0x13, int)
#define TIOCSPGRP _IOW(TTY_IOC_MAGIC, 0x20, pid_t)
#define TIOCGPGRP _IOR(TTY_IOC_MAGIC, 0x21, pid_t)
#define TIOCSCTTY _IOW(TTY_IOC_MAGIC, 0x22, int)
#define TIOCGSID _IOR(TTY_IOC_MAGIC, 0x23, pid_t)

#endif /* _UAPI_SYS_IOCTLS_H */