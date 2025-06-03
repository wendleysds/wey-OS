#ifndef _DISK_H
#define _DISK_H

#include <fs/fs.h>
#include <stdint.h>

struct Disk {
	uint32_t id;
	FN_OPEN open;
	FN_SEEK seek;
	FN_STAT stat;
	FN_READ read;
	FN_CLOSE close;
};

#endif
