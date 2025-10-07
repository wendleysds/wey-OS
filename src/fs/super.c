#include <wey/vfs.h>
#include <def/config.h>
#include <def/err.h>
#include <lib/string.h>
#include <mm/kheap.h>

static struct file_system_type* _registered_filesystems[FILESYSTEMS_MAX] = { 0x0 };
struct mount* mnt_root = 0x0;

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

int vfs_register_filesystem(struct file_system_type* fs){
    for (int i = 0; i < FILESYSTEMS_MAX; i++){
        if(!_registered_filesystems[i]){
            _registered_filesystems[i] = fs;
            return i;
        }
    }

    return OUT_OF_BOUNDS;
}

int vfs_unregister_filesystem(struct file_system_type* fs){
    for (int i = 0; i < FILESYSTEMS_MAX; i++){
        if(_registered_filesystems[i] == fs){
            _registered_filesystems[i] = 0x0;
            return i;
        }
    }

    return NOT_FOUND;
}

struct file_system_type* _find_fs_by_name(const char* name){
    for (int i = 0; i < FILESYSTEMS_MAX; i++)
    {
        if(strcmp(_registered_filesystems[i]->name, name) == 0){
            return _registered_filesystems[i];
        }
    }

    return 0x0;
}

struct super_block* alloc_super(){
	struct super_block* sb = (struct super_block*)kzalloc(sizeof(struct super_block));
	if(sb){
		INIT_LIST_HEAD(&sb->s_sbs);
		INIT_LIST_HEAD(&sb->s_inodes);
	}

	return sb;
}

int vfs_mount(const char* source, const char *mountpoint, const char *filesystemtype, void* data){
    if(!source || !mountpoint || !filesystemtype){
        return INVALID_ARG;
    }

    struct file_system_type* fs = _find_fs_by_name(filesystemtype);
    if(!fs){
        return NOT_FOUND;
    }

    struct mount* mnt = (struct mount*)kcalloc(sizeof(struct mount), 1);
    if(!mnt){
        return NO_MEMORY;
    }

    char* mts = strdup(mountpoint);
    if(!mts){
        kfree(mnt);
        return NO_MEMORY;
    }

	if(strncmp(source, "/dev", 3) == 0){
		source += 5;
	}

	struct inode* root = fs->mount(fs, 0x0, source, data);
    if(IS_ERR(root)){
        kfree(mnt);
        kfree(mts);

        return PTR_ERR(root);
    }

	mnt->mnt_root = root;	
    mnt->mnt_sb = (struct super_block *)root->i_sb;
    mnt->mountpoint = mts;

    _register_mount(mnt);
    return SUCCESS;
}

int vfs_umount(const char *mountpoint){
    struct mount* current = mnt_root;
    while (current){
        if(strcmp(current->mountpoint, mountpoint) == 0){
            int res;

            if((res = current->mnt_sb->fs_type->unmount(current->mnt_sb)) != SUCCESS){
                return res;
            }

            if((res = _unregister_mount(current)) != SUCCESS){
                return res;
            }

            kfree(current->mountpoint);
            kfree(current);

            return SUCCESS;
        }

        current = current->next;
    }

    return NOT_FOUND;
}