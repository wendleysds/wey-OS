#ifndef _BOOT_H
#define _BOOT_H

#include <stdint.h>

struct biosreg{};

void intcall(uint8_t int_no, const struct biosreg *inReg, struct biosreg *outReg);

#endif
