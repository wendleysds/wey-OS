#ifndef _CLIB_TERMIOS_H
#define _CLIB_TERMIOS_H

#include <bits/termios-c_lflags.h>
#include <bits/termios.h>
#include <sys/ioctls.h>

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

int tcgetattr(int fd, struct termios *termios);
int tcsetattr(int fd, int optional_actions, const struct termios *termios);

#endif /* _CLIB_TERMIOS_H */