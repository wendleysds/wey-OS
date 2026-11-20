#include <kernel/init.h>
#include <mm/page.h>
#include <mm/pagecache.h>
#include <def/errno.h>
#include <fs/vfs.h>
#include <fs/stat.h>
#include <fs/dcache.h>
#include <lib/string.h>

#include "ramfs_internal.h"

struct ramfs_sb {
	uint32_t next_ino;
};

static struct inode* ramfs_alloc_inode(struct super_block *sb) {
	struct ramfs_sb *rsb = sb->private_data;

	struct inode *inode = kzalloc(sizeof(struct inode));
	if(inode){
		struct ramfs_inode *rino = kzalloc(sizeof(struct ramfs_inode));
		if(!rino){
			kfree(inode);
			return NULL;
		}

		spinlock_init(&rino->spinlock);
		INIT_LIST_HEAD(&rino->children);
		INIT_LIST_HEAD(&rino->sibling);

		inode->ino = rsb->next_ino++;
		inode->private_data = rino;
		rino->ino = inode;
		inode->i_sb = sb;
	}

	return inode;
}

/* as páginas do arquivo já foram devolvidas por inode_destroy() -> pagecache_destroy() */
static void ramfs_destroy_inode(struct inode *inode) {
	if(inode->private_data){
		struct ramfs_inode *rino = inode->private_data;

		if(rino->name) {
			kfree(rino->name);
			rino->name = NULL;
		}

		kfree(rino);
	}

	kfree(inode);
}

static const struct super_operations ramfs_sops = {
	.alloc_inode = ramfs_alloc_inode,
	.destroy_inode = ramfs_destroy_inode,
};

static struct dentry* ramfs_mount(const struct file_system_type* self, const char* dev_name, void* data){
	struct super_block* sb = super_alloc();
	if (!sb) return ERR_PTR(-ENOMEM);

	struct ramfs_sb* rsb = kzalloc(sizeof(struct ramfs_sb));
	if (!rsb) {
		super_destroy(sb);
		return ERR_PTR(-ENOMEM);
	}
	rsb->next_ino = 1;
	sb->private_data = rsb;
	sb->fs_type = self;
	sb->s_op = &ramfs_sops;

	struct inode *root_inode = inode_new(sb);
	if (!root_inode) {
		kfree(rsb);
		super_destroy(sb);
		return ERR_PTR(-ENOMEM);
	}

	root_inode->mode = S_IFDIR;
	root_inode->i_op = &ramfs_iops;
	root_inode->i_fop = &ramfs_fops;
	root_inode->i_mapping->a_ops = &ramfs_aops;
	sb->root_inode = root_inode;

	struct dentry *root = d_make_root(root_inode);
	if (!root) {
		inode_destroy(root_inode);
		return ERR_PTR(-ENOMEM);
	}

	return root;
}

static void ramfs_unmount(struct super_block* sb){
	if(sb->private_data){
		kfree(sb->private_data);
		sb->private_data = NULL;
	}
}

static struct file_system_type ramfs = {
	.name = "ramfs",
	.mount = ramfs_mount,
	.unmount = ramfs_unmount
};

/*
 * Arquivo anônimo em memória (equivalente ao shmem_file_setup do Linux), usado por
 * mmap(MAP_SHARED | MAP_ANONYMOUS). Não tem nome em diretório algum: o inode nasce
 * com 1 referência, que passa a ser da `struct file`; quando a última VMA/fd solta
 * o arquivo, o inode morre e o page cache devolve as páginas.
 */
static struct super_block *shmem_sb;

struct file *shmem_file_setup(size_t size)
{
	if (!shmem_sb) {
		struct super_block *sb = super_alloc();
		if (!sb)
			return ERR_PTR(-ENOMEM);

		struct ramfs_sb *rsb = kzalloc(sizeof(struct ramfs_sb));
		if (!rsb) {
			super_destroy(sb);
			return ERR_PTR(-ENOMEM);
		}

		rsb->next_ino = 1;
		sb->private_data = rsb;
		sb->fs_type = &ramfs;
		sb->s_op = &ramfs_sops;
		shmem_sb = sb;
	}

	struct inode *inode = inode_new(shmem_sb);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	inode->mode = S_IFREG | 0600;
	inode->i_op = &ramfs_iops;
	inode->i_fop = &ramfs_fops;
	inode->i_mapping->a_ops = &ramfs_aops;
	inode->size = size;

	struct file *f = kzalloc(sizeof(struct file));
	if (!f) {
		inode_put(inode);
		return ERR_PTR(-ENOMEM);
	}

	f->inode = inode;
	f->f_op = inode->i_fop;
	f->flags = O_RDWR;
	atomic_set(&f->refcount, 1);
	return f;
}

static int ramfs_init(void){
	vfs_register_filesystem(&ramfs);
	return OK;
}

fs_initcall(ramfs_init);