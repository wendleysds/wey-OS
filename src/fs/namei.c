#include <fs/vfs.h>
#include <def/config.h>
#include <def/err.h>
#include <lib/mem.h>
#include <lib/string.h>

// /some/path/to/file.txt -> outdirs = "/some/path/to", outfname = "file.txt"
static inline void _extract_dirs_fname(char* path, char** outdirs, char** outfname) {
    if(!path) {
        return;
    }

    char *last_slash = strrchr(path, '/');

    *outfname = last_slash + 1;
    *last_slash = '\0'; // /some/path/to\0file.txt 
    *outdirs = path;
}

static struct inode* _get_dir(const char *restrict path) {
	if(!path || path[0] != '/' || strlen(path) > PATH_MAX) {
		return ERR_PTR(INVALID_ARG);
	}

	char pathcopy[PATH_MAX];
	strcpy(pathcopy, path);

	char* filename = 0x0;
	char* dirpath = 0x0;

	_extract_dirs_fname(pathcopy, &dirpath, &filename);

	if(strlen(filename) == 0) {
		return ERR_PTR(INVALID_ARG);
	}

	struct inode* dir = vfs_lookup(strlen(dirpath) > 0 ? dirpath : "/");
	if(IS_ERR(dir)) {
		return dir;
	}

	return dir;
}

int vfs_create(const char *restrict path, uint16_t mode){
    if(!path || path[0] != '/' || strlen(path) > PATH_MAX){
        return INVALID_ARG;
    }

    struct inode* dir = _get_dir(path);
	if(IS_ERR(dir)){
		return PTR_ERR(dir);
	}

	char* filename = strrchr(path, '/') + 1;
    int res = dir->i_op->create(dir, filename, mode);
    inode_destroy(dir);

    return res;
}

int vfs_unlink(const char *restrict path){
    if(!path || path[0] != '/' || strlen(path) > PATH_MAX){
        return INVALID_ARG;
    }

    struct inode* dir = _get_dir(path);
	if(IS_ERR(dir)){
		return PTR_ERR(dir);
	}

	char* filename = strrchr(path, '/') + 1;
    int res = dir->i_op->unlink(dir, filename);
    inode_destroy(dir);

    return res;
}

int vfs_mkdir(const char *restrict path){
    if(!path || path[0] != '/' || strlen(path) > PATH_MAX){
        return INVALID_ARG;
    }

    struct inode* dir = _get_dir(path);
	if(IS_ERR(dir)){
		return PTR_ERR(dir);
	}

	char* filename = strrchr(path, '/') + 1;
    int res = dir->i_op->mkdir(dir, filename);
    inode_destroy(dir);

    return res;
}

int vfs_rmdir(const char *restrict path){
    if(!path || path[0] != '/' || strlen(path) > PATH_MAX){
        return INVALID_ARG;
    }

    struct inode* dir = _get_dir(path);
	if(IS_ERR(dir)){
		return PTR_ERR(dir);
	}

	char* filename = strrchr(path, '/') + 1;
    int res = dir->i_op->rmdir(dir, filename);
    inode_destroy(dir);

    return res;
}
