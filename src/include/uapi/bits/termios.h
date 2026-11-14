#ifndef _UAPI_BITS_TERMIOS_H
#define _UAPI_BITS_TERMIOS_H

#include <stdint.h>

#define NCCS 31

typedef uint32_t tcflag_t;
typedef uint8_t cc_t;

struct termios {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_line;
	cc_t c_cc[NCCS];
};

struct winsize {
	uint16_t ws_row;
	uint16_t ws_col;
	uint16_t ws_xpixel;
	uint16_t ws_ypixel;
};

#endif /* _UAPI_BITS_TERMIOS_H */
