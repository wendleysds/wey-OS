#include <fs/vfs.h>
#include <def/errno.h>
#include <lib/string.h>
#include <uapi/dirent.h>

#include <uapi/sys/stat.h>
#include "ramfs_internal.h"

static char mode_to_dt(umode_t mode) {
    if (S_ISREG(mode))  return WEY_DT_REG;
    if (S_ISDIR(mode))  return WEY_DT_DIR;
    if (S_ISCHR(mode))  return WEY_DT_CHR;
    if (S_ISBLK(mode))  return WEY_DT_BLK;
    if (S_ISFIFO(mode)) return WEY_DT_FIFO;
    if (S_ISLNK(mode))  return WEY_DT_LNK;
    if (S_ISSOCK(mode)) return WEY_DT_SOCK;
    return WEY_DT_UNKNOWN;
}

    static int ramfs_dir_iterate(struct file *file, struct dir_context *ctx) {
        struct ramfs_inode *rdir = file->inode->private_data;

        if (!dir_emit_dots(file, ctx))
            return 0;

        spin_lock(&rdir->spinlock);

        off_t idx = 2;
        struct ramfs_dirent *de;
        list_for_each_entry(de, &rdir->children, sibling) {
            if (idx >= ctx->pos) {
                size_t name_len = strlen(de->name);
                char dt = mode_to_dt(de->ino->mode);

                if (dir_emit(ctx, de->name, name_len, de->ino->ino, dt) != 0) {
                    spin_unlock(&rdir->spinlock);
                    return 0;
                }
                ctx->pos++;
            }   
            idx++;
        }

        spin_unlock(&rdir->spinlock);
        return 0;
    }

    static off_t ramfs_dir_lseek(struct file *file, off_t offset, int whence) {
        off_t new_pos;

        switch (whence) {
            case SEEK_SET:
                new_pos = offset;
                break;
            case SEEK_CUR:
                new_pos = file->pos + offset;
                break;
            default:
                return -EINVAL;
        }

        if (new_pos < 0)
            return -EINVAL;

        file->pos = new_pos;
        return new_pos;
    }

    const struct file_operations ramfs_dir_fops = {
        .lseek = ramfs_dir_lseek,
        .iterate = ramfs_dir_iterate,
    };

