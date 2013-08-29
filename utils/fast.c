#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	/* PAGE_CACHE_SIZE */
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/blkdev.h>


#include "../print.h"

static struct super_operations fast_super_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static int fast_fill_super (struct super_block *sb, void *data, int silent)
{
	struct buffer_head *bh;
	struct inode *inode;
	
	sb->s_op = &fast_super_ops;
	PDEBUG("bdev_logical_block_size=%d\n", bdev_logical_block_size(sb->s_bdev));
	if(!sb_set_blocksize(sb, 512))
		return -EFAULT;
	
	sb->s_magic = 0xAAAAFFFF;
	
	PDEBUG("Fast Started.\n");
	bh = sb_bread(sb, 0);
	if(!bh) 
		return -EIO;
	
	PDEBUG("Read:     %c.\n", *(bh->b_data) );
	*bh->b_data += 1;
	PDEBUG("Written:  %c.\n", *(bh->b_data) );
	
	mark_buffer_dirty(bh);
	

	sync_dirty_buffer(bh);

	brelse(bh);
	
	inode = iget_locked(sb, 1);
	if(!inode)
		return -ENOMEM;
	
	inode->i_mode = S_IFDIR | 0;
	unlock_new_inode(inode);
	
	sb->s_root = d_make_root(inode);
	if(!sb->s_root) {
		iput(inode);
		return -ENOMEM;
	}
		
	return 0;
	
}
static struct dentry *fast_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
	return  mount_bdev(fs_type, flags, dev_name ,data, fast_fill_super);
}
static struct file_system_type fast_type = {
	.owner 		= THIS_MODULE,
	.name		= "fast",
	.mount		= fast_mount,
	.kill_sb	= kill_litter_super,
};


static int __init fast_init(void)
{	
	return register_filesystem(&fast_type);
}

static void __exit fast_exit(void)
{
	unregister_filesystem(&fast_type);
}
module_init(fast_init);
module_exit(fast_exit);
