#include <termios.h>
#include <sys/ioctl.h>

int tcgetattr(int fd, struct termios *termios) {
    return ioctl(fd, TCGETS, termios);
}