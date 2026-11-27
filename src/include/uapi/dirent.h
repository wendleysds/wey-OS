#ifndef _UAPI_DIRENT_H
#define _UAPI_DIRENT_H

#include <sys/types.h>

#define WEY_DT_UNKNOWN 0
#define WEY_DT_REG 1
#define WEY_DT_DIR 2
#define WEY_DT_LNK 3
#define WEY_DT_FIFO 4
#define WEY_DT_SOCK 5
#define WEY_DT_BLK 6
#define WEY_DT_CHR 7

struct wey_dirent {
    ino_t ino;
    off_t off;
    size_t reclen;
    char d_type;
    char name[];
};

#endif /* _UAPI_DIRENT_H */