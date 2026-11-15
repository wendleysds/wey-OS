#include <termios.h>
#include <sys/ioctl.h>

int tcsetattr(int fd, int optional_actions, const struct termios *termios){
    switch (optional_actions) {
        case TCSANOW:
            return ioctl(fd, TCSETS, termios);
        case TCSADRAIN:
            return ioctl(fd, TCSETSW, termios);
        case TCSAFLUSH:
            return ioctl(fd, TCSETSF, termios);
        default:
            return -1;
    }
}