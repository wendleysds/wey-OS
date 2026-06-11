#include <lib/string.h>
#include <def/config.h>
#include <def/err.h>
#include <fs/vfs.h>

extern struct mount *root_mount;

static void get_filename(const char* path, struct qstr* qstr){
	qstr->name = strrchr(path, '/') + 1;
	qstr->len = strlen(qstr->name);
}

static void get_directory_path(const char* path, struct qstr* qstr){
	const char *slash = strrchr(path, '/');

	qstr->name = path;
	qstr->len = (slash == path) ? 1 : slash - path;
}

static struct inode* get_dir(const char* path, struct qstr* qstr){
	get_directory_path(path, qstr);
	return vfs_lookup_qstr(qstr);
}

int vfs_create(const char *restrict path, uint16_t mode){
	if(!root_mount) return INVALID_STATE;
	struct qstr qstr;

	struct inode* dir = get_dir(path, &qstr);
	if(IS_ERR_OR_NULL(dir)){
		return PTR_ERR(dir);
	}

	get_filename(path, &qstr);

	int res = dir->i_op->create(dir, &qstr,mode);
	inode_put(dir);
	
	return res;
}

int vfs_unlink(const char *restrict path){
	if(!root_mount) return INVALID_STATE;
	struct qstr qstr;

	struct inode* dir = get_dir(path, &qstr);
	if(IS_ERR_OR_NULL(dir)){
		return PTR_ERR(dir);
	}

	get_filename(path, &qstr);
	
	int res = dir->i_op->unlink(dir, &qstr);
	inode_put(dir);
	return res;
}

int vfs_mkdir(const char *restrict path){
	if(!root_mount) return INVALID_STATE;
	struct qstr qstr;

	struct inode* dir = get_dir(path, &qstr);
	if(IS_ERR_OR_NULL(dir)){
		return PTR_ERR(dir);
	}

	get_filename(path, &qstr);
	
	int res = dir->i_op->mkdir(dir, &qstr);
	inode_put(dir);
	return res;
}

int vfs_rmdir(const char *restrict path){
	if(!root_mount) return INVALID_STATE;
	struct qstr qstr;

	struct inode* dir = get_dir(path, &qstr);
	if(IS_ERR_OR_NULL(dir)){
		return PTR_ERR(dir);
	}

	get_filename(path, &qstr);
	
	int res = dir->i_op->rmdir(dir, &qstr);
	inode_put(dir);
	return res;
}
