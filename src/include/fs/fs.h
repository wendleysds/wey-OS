#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

typedef void (*FN_OPEN)();
typedef void (*FN_SEEK)();
typedef void (*FN_READ)();
typedef void (*FN_STAT)();
typedef void (*FN_CLOSE)();

void fs_init();

#endif

