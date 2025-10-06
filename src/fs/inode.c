#include <fs/vfs.h>
#include <lib/list.h>
#include <core/kernel.h>
#include <memory/kheap.h>

struct inode* inode_alloc(struct super_block* sb){
	struct inode* inode;
	const struct super_operations* ops = sb->s_op;

	if (ops && ops->alloc_inode){
		inode = ops->alloc_inode(sb);
	}
	else{
		inode = (struct inode*)kzalloc(sizeof(struct inode));	
	}

	if(inode){
		inode->i_sb = sb;
		INIT_LIST_HEAD(&inode->i_sb_list);
	}

	return inode;
}

struct inode *inode_new(struct super_block *sb){
	struct inode* inode = inode_alloc(sb);
	if (inode){
		struct super_block *sb = inode->i_sb;
		list_add(&inode->i_sb_list, &sb->s_inodes);
	}

	return inode;
}

void inode_destroy(struct inode* inode){
	const struct super_operations* ops = inode->i_sb->s_op;
	if(ops && ops->destroy_inode){
		ops->destroy_inode(inode);
	}else{
		panic("inode_destroy(): No destroy operation on %s!", inode->i_sb->fs_type->name);
	}

	list_remove(&inode->i_sb_list);

	kfree(inode);
}
