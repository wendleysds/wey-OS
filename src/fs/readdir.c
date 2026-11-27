#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <kernel/sched.h>
#include <fs/fdtable.h>
#include <fs/vfs.h>
#include <def/errno.h>
#include <lib/string.h>

#include <uapi/sys/stat.h>
#include <uapi/dirent.h>
#include <stddef.h>

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

struct getdents_callback {
    struct dir_context ctx;
    char __user *buf;
    size_t count;
    size_t written;
    int error;
};

static int filldir(struct dir_context *ctx, const char *name, size_t len, ino_t ino, off_t off, char type) {
    struct getdents_callback *buf = (struct getdents_callback *)ctx;

    size_t name_off = offsetof(struct wey_dirent, name);
    size_t reclen = ALIGN_UP(name_off + len + 1, sizeof(long));

    if (reclen > (buf->count - buf->written)) {
        if (buf->written == 0)
            buf->error = -EINVAL;
        return -EINVAL;
    }

    struct wey_dirent d;
    memset(&d, 0, sizeof(d));
    d.ino = ino;
    d.off = off;
    d.reclen = reclen;
    d.d_type = type;

    char __user *dest = buf->buf + buf->written;

    if (copy_to_user(dest, &d, name_off)) {
        buf->error = -EFAULT;
        return -EFAULT;
    }

    if (copy_to_user(dest + name_off, name, len)) {
        buf->error = -EFAULT;
        return -EFAULT;
    }

    char zero = '\0';
    if (copy_to_user(dest + name_off + len, &zero, 1)) {
        buf->error = -EFAULT;
        return -EFAULT;
    }

    buf->written += reclen;
    return 0;
}

SYSCALL_DEFINE3(getdents, unsigned int, fd, __user struct wey_dirent *, dirent, unsigned int, count) {
    if (!dirent || count < sizeof(struct wey_dirent))
        return -EINVAL;

    struct file *file = file_table_get_file(current->files, fd);
    if (!file)
        return -EBADF;

    if (!S_ISDIR(file->inode->mode)) {
        file_put(file);
        return -ENOTDIR;
    }

    if (!file->f_op || !file->f_op->iterate) {
        file_put(file);
        return -ENOSYS;
    }

    struct getdents_callback cb = {
        .ctx = {
            .fill = filldir,
            .pos = file->pos,
        },
        .buf = (char __user *)dirent,
        .count = count,
        .written = 0,
        .error = 0,
    };

    int res = file->f_op->iterate(file, &cb.ctx);
    file->pos = cb.ctx.pos;

    file_put(file);

    if (cb.error)
        return cb.error;

    if (res < 0)
        return res;

    return cb.written;
}