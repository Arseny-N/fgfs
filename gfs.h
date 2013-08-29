#ifndef _GFS_HDR_
#define _GFS_HDR_

#include <linux/rwsem.h>
#include <linux/kobject.h>        /* struct kobject */
#include <linux/buffer_head.h>
#include <linux/mutex.h>

#include "../print.h"

#include <uapi/linux/types.h>
#include "structs.h"


struct gfs_inode_info {
	struct inode vfs_inode;

	u16  i_flags;
	u32  i_exino;	

	struct gfs_inode_info *i_ex_inode;
	
	struct rw_semaphore i_rwsem;
	struct buffer_head *i_bh;
	
};


#define gfs_wrn(fmt, args...) printk(KERN_WARNING KBUILD_MODNAME ": %s() " fmt "\n", __func__, ## args)
#define gfs_dbg(fmt, args...) printk(KERN_DEBUG KBUILD_MODNAME ": %s() " fmt "\n", __func__, ## args)
#define gfs_info(fmt, args...) printk(KERN_INFO KBUILD_MODNAME ": %s() " fmt "\n", __func__, ## args)
#define gfs_fatal(fmt, args...) printk(KERN_ERR KBUILD_MODNAME ": %s() " fmt "\n", __func__, ## args)


#define VFS_SUPER(s) ((s)->s_vfs_sb)
#define GFS_SUPER(s) ((s)->s_fs_info)

#define DATA_START(i) ((i)->i_bh->b_data +sizeof(struct gfs_inode))

struct gfs_dir_info {

	struct gfs_dir *raw_dir;
	struct gfs_inode_info *dir;
	
	/* Do not touch */
	char *addr;
	int blocksize;      
	int chunksize;
	int valid;
	int off;          
	int write;
	int block;        

	
};
struct gfs_super_info {
	struct super_block *s_vfs_sb; 
	struct gfs_inode_info  *s_root_inode;
	struct gfs_inode_info *s_super_inode;
	
	unsigned int s_chunk_size;  /* Inclusive, in blocks, i.e. if 5 than 5 blocks. */
	unsigned int s_blocksize; 	
	
	u32 s_nchunks;
	u32 s_nfree_chunks;

	u32 s_ino_root;
	u8 *s_inode_map;
	struct kobject s_kobj;
	struct mutex s_mutex;

		

};


static inline struct inode *VFS_INODE(struct gfs_inode_info *i)
{
	return (struct inode *) &(i->vfs_inode);
}
static inline struct gfs_inode_info *GFS_INODE(struct inode *i)
{
	return (struct gfs_inode_info *) (container_of((i), struct gfs_inode_info, vfs_inode));
}
static inline struct gfs_inode *RAW_INODE(struct gfs_inode_info *i)
{
	return ((struct gfs_inode *) i->i_bh->b_data);
}
static inline unsigned int get_chunk_size(struct gfs_inode_info *i)
{
	return ((struct gfs_super_info *) i->vfs_inode.i_sb->s_fs_info)->s_chunk_size;
}

/* file.c */

extern struct address_space_operations gfs_aops;
extern struct file_operations gfs_file_operations;

/* dir.c */

extern struct file_operations gfs_dir_operations;
extern struct inode_operations gfs_dir_inode_operations;

/* sysfs.c */

extern int gfs_sysfs_init(void);
extern void gfs_sysfs_exit(void);

extern int gfs_sysfs_init_sb(struct gfs_super_info *sbi);
extern void gfs_sysfs_exit_sb(struct gfs_super_info *si);


static inline void __print_inode(const char *func, struct gfs_inode_info *i)
{
	__PDEBUG(__FILE__, func, __LINE__,"print inode: ino=%ld, id=%ld, size=%ld, time=%ld, mode=%lo, exino=%lx\n", 
		(long) VFS_INODE(i)->i_ino, 
		(long) VFS_INODE(i)->i_uid,
		(long) VFS_INODE(i)->i_size,
		(long) VFS_INODE(i)->i_atime.tv_sec,
		(long) VFS_INODE(i)->i_mode,
		(long) i->i_exino);
}
#define print_inode(inode) __print_inode(__func__,inode)

/* inode.c */

static inline void lock_inode_data(struct gfs_inode_info *i, int write)
{
	PDEBUG("Lock %i\n", write);
	if(write) 
		down_write(&i->i_rwsem);
	else      
		down_read(&i->i_rwsem);
}
static inline void unlock_inode_data(struct gfs_inode_info *i, int write)
{
	PDEBUG("UnLock %i\n", write);
	if(write)
		up_write(&i->i_rwsem);
	else
		up_read(&i->i_rwsem);
}

static inline struct buffer_head *get_inode_block(struct gfs_inode_info *i, loff_t boff,int write)
{
	int off;
	
	lock_inode_data(i, write);	
	WARN_ON(i->i_bh);
	
	off = (VFS_INODE(i)->i_ino*get_chunk_size(i))+boff;
	
	i->i_bh = sb_bread(i->vfs_inode.i_sb, off);
	if(!i->i_bh) 
		unlock_inode_data(i, write);	
	return i->i_bh;
}
static inline struct buffer_head *get_inode_data(struct gfs_inode_info *i, int write)
{
	return get_inode_block(i, 0, write);
}

static inline void __sync_inode_data(const char *func, struct gfs_inode_info *i)
{
	PDEBUG("func:%s\n", func);
	WARN_ON(!rwsem_is_locked(&i->i_rwsem));
	mark_buffer_dirty(i->i_bh);
	sync_dirty_buffer(i->i_bh);	
}
#define sync_inode_data(i) __sync_inode_data(__func__, i)
static inline void put_inode_block(struct gfs_inode_info *i, int write)
{
	sync_inode_data(i);
	brelse(i->i_bh);
	i->i_bh = NULL;
	unlock_inode_data(i, write);

}
static inline void put_inode_data(struct gfs_inode_info *i, int write)
{
	put_inode_block(i, write);
}
static inline void gfs_dirty_super(struct super_block *sb)
{
	struct gfs_super_info *super = sb->s_fs_info;
	PDEBUG("Dirty\n");
	mark_inode_dirty_sync(VFS_INODE(super->s_super_inode));
	
}
extern int gfs_write_inode(struct inode *inode, struct writeback_control *wbc);

static inline void gfs_sync_super(struct super_block *sb)
{
	struct gfs_super_info *super = sb->s_fs_info;
	PDEBUG("SUPER\n");
	gfs_write_inode (VFS_INODE(super->s_super_inode), NULL);
	//if(get_inode_data(super->s_inode, 1)) {
	//	sync_inode_data(super->s_inode);
	//	put_inode_data(super->s_inode,1);
	//}
	//gfs_dirty_super(sb);
}





extern struct inode *gfs_iget_new(struct gfs_super_info *sb);
extern struct inode *gfs_iget(struct super_block *sb, u32 ino);
extern void gfs_put_inode(struct gfs_inode_info *i);

extern void gfs_init_inode(struct inode *i, struct dentry *parent ,u16 mode); 








#endif /* _GFS_HDR_*/ 
