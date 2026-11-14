#ifndef _UAPI_BITS_TERMIOS_C_LFLAGS_H
#define _UAPI_BITS_TERMIOS_C_LFLAGS_H

#ifndef BIT
#define BIT(n) (1U << (n))
#endif

// c_iflags
#define IGNBRK  BIT(0)
#define BRKINT  BIT(1)
#define IGNPAR  BIT(2)
#define PARMRK  BIT(3)
#define INPCK   BIT(4)
#define ISTRIP  BIT(5)
#define INLCR   BIT(6)
#define IGNCR   BIT(7)
#define ICRNL   BIT(8)
#define IXON    BIT(9)
#define IXOFF   BIT(10)

// Legacy aliases
#define INPCAP_IGNBRK IGNBRK

// c_oflags
#define OPOST   BIT(0)
#define ONLCR   BIT(1)
#define OCRNL   BIT(2)
#define ONOCR   BIT(3)
#define ONLRET  BIT(4)

// Legacy aliases
#define OFLAG_OPOST OPOST

// c_cflags
#define CS5     BIT(0)
#define CS6     BIT(1)
#define CS7     BIT(2)
#define CS8     BIT(3)
#define CSIZE   (CS5 | CS6 | CS7 | CS8)
#define CSTOPB  BIT(4)
#define CREAD   BIT(5)
#define PARENB  BIT(6)
#define PARODD  BIT(7)
#define HUPCL   BIT(8)
#define CLOCAL  BIT(9)

// Legacy aliases
#define CFLAG_CSIZE CSIZE
#define CFLAG_CS5   CS5
#define CFLAG_CS6   CS6
#define CFLAG_CS7   CS7
#define CFLAG_CS8   CS8

// c_lflags
#define ISIG    BIT(0)
#define ICANON  BIT(1)
#define ECHO    BIT(2)
#define ECHOE   BIT(3)
#define ECHOK   BIT(4)
#define ECHONL  BIT(5)
#define NOFLSH  BIT(6)
#define TOSTOP  BIT(7)
#define IEXTEN  BIT(8)
#define ECHOCTL BIT(9)

// Legacy aliases
#define LFLAG_ECHO   ECHO
#define LFLAG_ICRNOC BIT(9)

// c_cc
#define VINTR    0
#define VQUIT    1
#define VERASE   2
#define VKILL    3
#define VEOF     4
#define VTIME    5
#define VMIN     6
#define VSWTC    7
#define VSTART   8
#define VSTOP    9
#define VSUSP    10
#define VEOL     11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE  14
#define VLNEXT   15
#define VEOL2    16

#endif /* _UAPI_BITS_TERMIOS_C_LFLAGS_H */