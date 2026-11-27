#include <dirent.h>
#include <syscall.h>
#include <string.h>
#include <errno.h>
#include "dirstream.h"

struct dirent *readdir(DIR *dir) {
    if (!dir) {
        errno = EBADF;
        return NULL;
    }

    while (1) {
        if (dir->buf_pos >= dir->buf_end) {
            long nread = syscall(SYS_getdents, dir->fd, dir->buf, sizeof(dir->buf));
            if (nread <= 0) {
                return NULL;
            }
            dir->buf_pos = 0;
            dir->buf_end = (size_t)nread;
        }

        struct wey_dirent *wde = (struct wey_dirent *)(dir->buf + dir->buf_pos);
        if (wde->reclen == 0) {
            return NULL;
        }

        dir->buf_pos += wde->reclen;
        dir->tell = wde->off;

        dir->dent.d_ino = wde->ino;
        dir->dent.d_off = wde->off;
        dir->dent.d_reclen = (unsigned short)wde->reclen;
        dir->dent.d_type = (unsigned char)wde->d_type;

        size_t name_len = strlen(wde->name);
        if (name_len > NAME_MAX) {
            name_len = NAME_MAX;
        }
        memcpy(dir->dent.d_name, wde->name, name_len);
        dir->dent.d_name[name_len] = '\0';

        return &dir->dent;
    }
}
