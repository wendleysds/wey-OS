#include <kernel/init.h>
#include <mm/page.h>
#include <def/err.h>
#include <fs/vfs.h>
#include <fs/stat.h>
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

		rino->page_capacity = 0;
		rino->pages = NULL;

		inode->ino = rsb->next_ino++;
		inode->private_data = rino;
		rino->ino = inode;
		inode->i_sb = sb;
	}

	return inode;
}

static void ramfs_destroy_inode(struct inode *inode) {
	if(inode->private_data){
		struct ramfs_inode *rino = inode->private_data;

		if(rino->name) {
			kfree(rino->name);
			rino->name = NULL;
		}

		if(rino->pages){
			for(size_t i = 0; i < rino->page_capacity; i++){
				if(rino->pages[i]){
					page_free(rino->pages[i]);
				}

				rino->pages[i] = NULL;
			}

			kfree(rino->pages);
			rino->pages = NULL;
		}

		kfree(rino);
	}
}

static void ramfs_free_inode(struct inode *inode) {
	kfree(inode);
}

static const struct super_operations ramfs_sops = {
	.alloc_inode = ramfs_alloc_inode,
	.destroy_inode = ramfs_destroy_inode,
	.free_inode = ramfs_free_inode
};

static struct inode* ramfs_mount(const struct file_system_type* self, const char* dev_name, void* data){
	struct super_block* sb = super_alloc();
	if(!sb){
		return ERR_PTR(NO_MEMORY);
	}

	struct ramfs_sb* rsb = kzalloc(sizeof(struct ramfs_sb));
	if(!rsb){
		kfree(sb);
		return ERR_PTR(NO_MEMORY);
	}

	rsb->next_ino = 1;

	sb->private_data = rsb;
	sb->fs_type = self;
	sb->s_op = &ramfs_sops;

	struct inode *root = inode_new(sb);
	if(!root){
		kfree(sb);
		kfree(rsb);
		return ERR_PTR(NO_MEMORY);
	}

	root->mode = S_IFDIR;

	root->i_op = &ramfs_iops;
	root->i_fop = &ramfs_fops;

	sb->root_inode = root;

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

static int ramfs_init(void){
	vfs_register_filesystem(&ramfs);
	return OK;
}

fs_initcall(ramfs_init);
