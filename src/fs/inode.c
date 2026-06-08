#include <kernel/panic.h>
#include <fs/vfs.h>

struct inode* inode_alloc(struct super_block* sb){
	struct inode* inode = NULL;

	if (sb->s_op && sb->s_op->alloc_inode){
		inode = sb->s_op->alloc_inode(sb);
	}
	else{
		panic("inode_alloc(): No alloc operation on %s!", sb->fs_type->name);
	}

	return inode;
}

struct inode* inode_new(struct super_block *sb){
	struct inode* inode = inode_alloc(sb);
	if (inode){
		inode->i_sb = sb;
		INIT_LIST_HEAD(&inode->i_sb_list);

		spinlock_init(&inode->lock);
		atomic_set(&inode->refcount, 1);

		spin_lock(&sb->s_inode_lock);
		list_add(&inode->i_sb_list, &sb->s_inodes);
		spin_unlock(&sb->s_inode_lock);
	}

	return inode;
}

void inode_destroy(struct inode* inode){
	const struct super_operations* ops = inode->i_sb->s_op;
	if(ops && ops->destroy_inode){
		spin_lock(&inode->i_sb->s_inode_lock);
		list_remove(&inode->i_sb_list);
		spin_unlock(&inode->i_sb->s_inode_lock);
		ops->destroy_inode(inode);
	}else{
		panic("inode_destroy(): No destroy operation on %s!", inode->i_sb->fs_type->name);
	}
}
