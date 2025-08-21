#include <fs/vfs.h>
#include <def/config.h>
#include <def/status.h>
#include <lib/string.h>
#include <memory/kheap.h>

static struct filesystem* _registered_filesystems[FILESYSTEMS_MAX] = { 0x0 };
struct mount* mnt_root = 0x0;
struct inode* vfs_root_node = 0x0;

static inline void _register_mount(struct mount* mount){
    mount->next = 0x0;

    if(!mnt_root){
        mnt_root = mount;
        return;
    }

    struct mount* n = mnt_root->next;
    mnt_root->next = mount;
    mount->next = n;
}

static inline int _unregister_mount(struct mount* mount){
    struct mount* current = mnt_root;
    if(current == mount){
        mnt_root = mnt_root->next;
        return SUCCESS;
    }

    while (current){
        struct mount* next = current->next;
        if(next == mount){
            current->next = mount->next;
            return SUCCESS;
        }

        current = next;
    }

    return NOT_FOUND;
}

int vfs_register_filesystem(struct filesystem* fs){
    for (int i = 0; i < FILESYSTEMS_MAX; i++){
        if(!_registered_filesystems[i]){
            _registered_filesystems[i] = fs;
            return i;
        }
    }

    return OUT_OF_BOUNDS;
}

int vfs_unregister_filesystem(struct filesystem* fs){
    for (int i = 0; i < FILESYSTEMS_MAX; i++){
        if(_registered_filesystems[i] == fs){
            _registered_filesystems[i] = 0x0;
            return i;
        }
    }

    return NOT_FOUND;
}

struct filesystem* _find_fs_by_name(const char* name){
    for (int i = 0; i < FILESYSTEMS_MAX; i++)
    {
        if(strcmp(_registered_filesystems[i]->name, name) == 0){
            return _registered_filesystems[i];
        }
    }

    return 0x0;
}

int vfs_mount(struct device *device, const char *mountpoint, const char *filesystemtype){
    if(!device || !mountpoint || !filesystemtype){
        return INVALID_ARG;
    }

    struct filesystem* fs = _find_fs_by_name(filesystemtype);
    if(!fs){
        return NOT_FOUND;
    }

    struct superblock* sb = (struct superblock*)kmalloc(sizeof(struct superblock));
    if(!sb){
        return NO_MEMORY;
    }

    struct mount* mnt = (struct mount*)kcalloc(sizeof(struct mount), 1);
    if(!mnt){
        kfree(sb);
        return NO_MEMORY;
    }

    char* mts = strdup(mountpoint);
    if(!mts){
        kfree(sb);
        kfree(mnt);
        return NO_MEMORY;
    }

    int res = fs->mount(sb, device);
    if(res != SUCCESS){
        kfree(sb);
        kfree(mnt);
        kfree(mts);

        return res;
    }

    if(!vfs_root_node){
        vfs_root_node = fs->get_root(sb);
    }

    mnt->sb = sb;
    mnt->fs = fs;
    mnt->mountpoint = mts;

    _register_mount(mnt);
    return SUCCESS;
}

int vfs_umount(const char *mountpoint){
    struct mount* current = mnt_root;
    while (current){
        if(strcmp(current->mountpoint, mountpoint) == 0){
            int res;

            if((res = current->fs->unmount(current->sb)) != SUCCESS){
                return res;
            }

            if((res = _unregister_mount(current)) != SUCCESS){
                return res;
            }

            kfree(current->sb);
            kfree(current->mountpoint);
            kfree(current);

            return SUCCESS;
        }

        current = current->next;
    }

    return NOT_FOUND;
}