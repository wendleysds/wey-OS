#include <fs/vfs.h>
#include <def/err.h>
#include <memory/kheap.h>
#include <lib/mem.h>
#include <lib/string.h>
#include <def/config.h>

static struct inode *_find_mount_for_path(const char *path, const char **out_relative){
    size_t best_match_len = 0;
	struct mount* current = mnt_root;
    struct inode *result = current->mnt_root;

    while(current){
        const char* mntPath = current->mountpoint;
        size_t len = strlen(mntPath);  

        if(strncmp(path, mntPath, len) == 0 && (path[len] == '/' || path[len] == '\0')){
            if(len > best_match_len){
                best_match_len = len;
                result = current->mnt_root;
                *out_relative = (path[len] == '/') ? path + len + 1 : path + len;
            }
        }
        
        current = current->next;
    }

    if(best_match_len == 0){
        *out_relative = path;
    }

    return result;
}

static inline struct inode* vfs_traverse_path(const char* path){
    if(!path || path[0] != '/' || strlen(path) > PATH_MAX){
        return ERR_PTR(INVALID_ARG);
    }

    const char *relative_path;
    struct inode *root = _find_mount_for_path(path, &relative_path);

    char pathcopy[PATH_MAX];
    memset(pathcopy, 0x0, sizeof(pathcopy));
    strncpy(pathcopy, relative_path, PATH_MAX);

    pathcopy[PATH_MAX - 1] = '\0';

    struct inode *current = root;
    struct inode *next = NULL;

    char *token = strtok(pathcopy, "/");
    while(token){
        if (!current->i_op || !current->i_op->lookup)
            return ERR_PTR(NOT_SUPPORTED);

        next = current->i_op->lookup(current, token);
        if (IS_ERR(next)) {
            return next;
        }

        if (current != root)
            inode_dispose(current);
        
        current = next;
        token = strtok(NULL, "/");
    }

    if(!current->i_fop || !current->i_fop->read){
        if (current != root){
            inode_dispose(current);
        }

        return ERR_PTR(INVALID_FILE);
    }

    return current;
}

struct inode* vfs_lookup(const char *restrict path){
    return vfs_traverse_path(path);
}
