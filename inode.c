#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include "gfs.h"

/*
 * Boilerplate stuff.
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jonathan Corbet");


#define GFS_MAGIC 0xBBBEEEFF  /* So Hungry ;-) */


/*
 * Anytime we make a file or directory in our filesystem we need to
 * come up with an inode to represent it internally.  This is
 * the function that does that job.  All that's really interesting
 * is the "mode" parameter, which says whether this is a directory
 * or file, and gives the permissions.
 */
static void init_raw_from_super(struct gfs_super *raw, struct gfs_super_info *sb);




#define TO_TIMESPEC(s,n) ((struct timespec){ (unsigned long) s, (unsigned long) n})

/* Inode operations */
static void gfs_init_inode_always(struct gfs_inode_info *i)
{
	
	init_rwsem(&i->i_rwsem);
	i->i_bh = NULL;
}

static void gfs_set_inode(struct inode *inode)
{
	inode->i_mapping->a_ops = &gfs_aops;

	
	switch ( inode->i_mode & S_IFMT) {
	case S_IFREG:
		inode->i_fop = &gfs_file_operations;	
			
		break;
	case S_IFDIR:
		inode->i_fop = &gfs_dir_operations;
		inode->i_op = &gfs_dir_inode_operations;

		set_nlink(inode, inode->i_nlink + 2);

		break;
	}
}

void gfs_init_inode(struct inode *inode, struct dentry *parent, u16 mode )
{	
	struct gfs_inode_info *gfs_i = GFS_INODE(inode);
	

	
	inode_init_owner(inode, parent->d_inode, mode);	
	inode->i_blocks = 1;	
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	inode->i_mode = mode;
	inode->i_size = 0;
	gfs_set_inode(inode);
	gfs_i->i_exino = 0xf8f8;
	

	print_inode(gfs_i);
}
EXPORT_SYMBOL(gfs_init_inode);


static inline void gfs_init_inode_from_raw (struct gfs_inode_info *i, struct gfs_inode *raw)
{
	struct inode *in = VFS_INODE(i);

	
	set_nlink(in, le16_to_cpu(raw->i_nlink));
	
	in->i_mode = le16_to_cpu(raw->i_mode);
	in->i_atime.tv_sec = le32_to_cpu(raw->i_time);
	in->i_uid = le16_to_cpu(raw->i_id);
	in->i_size = le16_to_cpu(raw->i_size);
	
	i->i_flags = le16_to_cpu(raw->i_flags);
	i->i_exino = le32_to_cpu(raw->i_exino);
	gfs_set_inode(in);

}

static inline void gfs_init_raw_from_inode (struct gfs_inode *raw,struct gfs_inode_info *i)
{
	struct inode *in = VFS_INODE(i);

	raw->i_nlink = cpu_to_le16(in->i_nlink);
	raw->i_mode = cpu_to_le32(in->i_mode);
	raw->i_time = cpu_to_le16(in->i_atime.tv_sec);
	raw->i_id = cpu_to_le16(in->i_uid);
	raw->i_size = cpu_to_le16(in->i_size);
	raw->i_flags =  cpu_to_le16(i->i_flags);

}





static struct gfs_inode_info *gfs_iget_info(struct super_block *sb, u32 ino) 
{
	struct gfs_inode_info *gfs_inode;
	struct gfs_inode  *raw_inode;
	struct buffer_head   *bh;
	struct inode  *vfs_inode;
	PDEBUG("ino=%d\n", (int)ino);
	vfs_inode = iget_locked(sb, ino);
	if(!vfs_inode) 
		return ERR_PTR(-ENOMEM);
	

	gfs_inode = GFS_INODE(vfs_inode);
	
	if(vfs_inode->i_state &  I_NEW) {
		bh = get_inode_data(gfs_inode, 0);
		if(!bh) {
			PDEBUG("IO\n");
			return ERR_PTR(-EIO);
		}
		
		raw_inode = (struct gfs_inode*)bh->b_data;
		PDEBUG("Raw Inode %o\n", raw_inode->i_mode);
		
		gfs_init_inode_from_raw(gfs_inode, RAW_INODE(gfs_inode));

		gfs_inode->i_ex_inode = NULL;
		gfs_inode->i_exino = 0xabcd;


		
		put_inode_data(gfs_inode, 0);
		unlock_new_inode(vfs_inode);

	}
	print_inode(gfs_inode);
	return gfs_inode;
}
struct inode *gfs_iget(struct super_block *sb, u32 ino)
{
	struct gfs_inode_info *gi = gfs_iget_info(sb, ino);
	return ! IS_ERR(gi) ? VFS_INODE(gi) : NULL;
}
EXPORT_SYMBOL(gfs_iget);
#if 0
static int freeInode = 3;
static struct gfs_inode *_gfs_find_free(struct gfs_super_info *sb, struct buffer_head **bh, u32 *num)
{
	int i = freeInode++;

	struct gfs_inode *inode = gfs_read_inode(VFS_SUPER(sb), i, bh);
	
	if(inode && !(le32_to_cpu(inode->i_flags) & GI_INO_USED)){ 
		*num = i;
		return inode;
	}
		brelse(*bh);

	BUG();
	return NULL;
}

static struct gfs_inode *gfs_new_inode_raw(struct gfs_super_info *sb, struct buffer_head **bhp, u32 *num)
{
	struct gfs_inode *inode = NULL;
	mutex_lock(&sb->s_mutex);
	
	if(sb->s_nfree_chunks== 0)
		goto out;
	
	
	inode = _gfs_find_free(sb, bhp, num);
	if(inode) 
		sb->s_nfree_chunks--;
	
out:
	mutex_unlock(&sb->s_mutex);
	
	return inode;
	
}
int gfs_junk_inode(struct gfs_inode *inode)
{	
	if(!get_inode_data(inode, 1))
		return -ENOMEM;
	BUG_ON(inode->i_flags & GI_INO_JUNK);
	inode->i_flags |= GI_INO_JUNK;
	mark_inode_dirty(VFS_INODE(inode));
	
	put_inode_data(inode, 1);
	return 0;
}
#endif
static int gfs_zero_inode(struct gfs_inode_info *inode)
{
	struct gfs_super_info *sb = (struct gfs_super_info *)inode->vfs_inode.i_sb->s_fs_info;
	int nblocks = sb->s_chunk_size, i;
	
	for( i = 0;i<nblocks; ++i ) {
		struct buffer_head *bh = get_inode_block(inode, i, 1);
		if(!bh)
			return -ENOMEM;
		memset(bh->b_data, 0, bh->b_size);
		put_inode_block(inode,1);
	}
	return 0;
}


/* Routines handling inode numbers */


static u32 grab_free_ino(struct gfs_super_info *sb, bool *err)
{
	u32 ino = 0, i;

	mutex_lock(&sb->s_mutex);
	
	if(sb->s_nfree_chunks) {
		
		for( i = 0; i < sb->s_nchunks; ++i) 
			if( sb->s_inode_map[i] != 0xff) {
				ino = first_zero_bit(sb->s_inode_map[i]) + 8*i;			
				break;
			}

		__mark_bit_as_used(sb->s_inode_map, ino);
		sb->s_nfree_chunks--;
		*err = 0;
	} else {
		*err = 1;
	}
	
	mutex_unlock(&sb->s_mutex);
	
	gfs_sync_super(sb->s_vfs_sb);
	return ino;
}
static void release_ino(struct gfs_super_info *sb, u32 ino)
{
	BUG_ON(ino >= sb->s_nchunks);
	gfs_dbg("ino=%d", ino);
	mutex_lock(&sb->s_mutex);
	
	BUG_ON(!__is_bit_used(sb->s_inode_map, ino));
	__mark_bit_as_unused(sb->s_inode_map, ino);
	sb->s_nfree_chunks++;
	
	mutex_unlock(&sb->s_mutex);
	gfs_sync_super(sb->s_vfs_sb);
}
void gfs_put_inode(struct gfs_inode_info *inode)
{
	release_ino(inode->vfs_inode.i_sb->s_fs_info, inode->vfs_inode.i_ino);
	iput(&inode->vfs_inode);
}
EXPORT_SYMBOL(gfs_put_inode);
struct inode *gfs_iget_new(struct gfs_super_info *sb)
{
	struct gfs_inode_info *inode;
	bool err = 0;
	u32 ino = grab_free_ino(sb, &err);
	if(err) {
		gfs_info("No more free space\n");
		return ERR_PTR(-ENOMEM);            /* TODO: Really ENOMEM ?*/
	}
	BUG_ON(!__is_bit_used(sb->s_inode_map, ino));
	inode = gfs_iget_info(sb->s_vfs_sb, ino);
	if(IS_ERR(inode)) {
		PDEBUG("get info\n");
		return ERR_CAST(inode);
	}
			
	BUG_ON(inode->vfs_inode.i_state & I_NEW);
	
	if(!get_inode_data(inode,1))
		goto err;
	RAW_INODE(inode)->i_flags = cpu_to_le32(GI_INO_USED);
	RAW_INODE(inode)->i_nlink = cpu_to_le32(1);
	set_nlink(VFS_INODE(inode), 1);
	put_inode_data(inode, 1);

	gfs_dbg("New ino: %ld %i\n",VFS_INODE(inode)->i_ino, VFS_INODE(inode)->i_nlink);
	
	return VFS_INODE(inode);
err:
	gfs_put_inode(inode);
	return ERR_PTR(-EIO);
}
EXPORT_SYMBOL(gfs_iget_new);

/* Inode data operations */

/* 	Super operations */




/* Vfs Methods (operations) */
/* 	Directory inode operations */





/*
 * Superblock stuff.  This is all boilerplate to give the vfs something
 * that looks like a filesystem to work with.
 */

/*
 * Our superblock operations, both of which are generic kernel ops
 * that we don't have to write ourselves.
 */

 
static struct inode *gfs_alloc_inode(struct super_block *sb)
{
	struct gfs_inode_info *i = kzalloc(sizeof(struct gfs_inode_info), GFP_NOFS);

	if(!i)	
		return NULL;
	i->vfs_inode.i_version = 1;
	gfs_init_inode_always(i);
	
	inode_init_once(&(i->vfs_inode));
	return &(i->vfs_inode);
}
static int gfs_drop_inode(struct inode *inode)
{

	int r = generic_drop_inode(inode);

	PDEBUG("Dropping inode %ld,r=%d,mode=%o,r=%i\n", inode->i_ino, r, inode->i_mode, r);
	return r;
	
}
static void gfs_destroy_inode(struct inode *inode)
{
	struct gfs_inode_info *i = GFS_INODE(inode);
	PDEBUG("Delitting inode %ld\n", inode->i_ino);
	kfree(i);
	
}
int gfs_write_inode(struct inode *inode, struct writeback_control *wbc)
{
	struct gfs_inode_info *gfs_inode = GFS_INODE(inode);
	struct gfs_inode *raw_inode;
	struct buffer_head *bh = get_inode_data(gfs_inode, 1);
	if(!bh) {
		PDEBUG("-EIO\n");
		return -EIO;
	}
	
	raw_inode = (struct gfs_inode *)bh->b_data;
	
	if(inode->i_ino == 0) {
		struct gfs_super *raw_super = (struct gfs_super *) (bh->b_data + sizeof(struct gfs_inode));
		init_raw_from_super(raw_super, inode->i_sb->s_fs_info);
	}

	gfs_init_raw_from_inode(raw_inode, gfs_inode);
	PDEBUG("Writing inode %ld %o %o\n", inode->i_ino, le32_to_cpu(raw_inode->i_mode), inode ->i_mode);	
	
	sync_inode_data(gfs_inode);
	put_inode_data(gfs_inode, 1);
	
	return 0;
}
EXPORT_SYMBOL(gfs_write_inode);
static struct super_operations gfs_s_ops = {
	.alloc_inode    = gfs_alloc_inode ,
	.statfs		= simple_statfs,
//	.drop_inode	= gfs_drop_inode,
	.destroy_inode	= gfs_destroy_inode,
	.write_inode 	= gfs_write_inode,
};

/*
 * "Fill" a superblock with mundane stuff.
 */
static void init_raw_from_super(struct gfs_super *raw, struct gfs_super_info *sb)
{
	int i, map_size;

	mutex_lock(&sb->s_mutex);
	
#define S(n) raw->n = cpu_to_le32(sb->n)
#define _S(n) ({ const typeof(S(n)) __m = S(n); PDEBUG("%s=%ld\n",#n,(long)__m);  __m;})
	//_S(s_nchunks);
	S(s_ino_root);
	//raw->s_blocksize = sb->s_blocksize;
#undef S
#undef _S		
	map_size = __count_map_size(sb->s_nchunks);
	
	for(i=0 ; i < map_size; ++i)  {
		raw->s_inode_map[i] = sb->s_inode_map[i];	
	
	}
	
	mutex_unlock(&sb->s_mutex);
	
}

static int init_super_from_raw(struct gfs_super_info *g, struct gfs_super *s)
{
	int i, map_size;
	u8 map = 0;
	mutex_init(&g->s_mutex);
	mutex_lock(&g->s_mutex);
	g->s_nfree_chunks= 0;
	
#define S(n) g->n = le32_to_cpu(s->n)
#define _S(n) ({ const typeof(S(n)) __m = S(n); PDEBUG("%s=%ld\n",#n,(long)__m);  __m;})
	S(s_nchunks);
	S(s_ino_root);
	g->s_chunk_size = s->s_chunk_size;
	map_size = __count_map_size(g->s_nchunks);

	g->s_inode_map = kzalloc(map_size, GFP_KERNEL);
	if(!g->s_inode_map) {
		mutex_unlock(&g->s_mutex);
		return -ENOMEM;
	}

	for(i=0 ; i < map_size; ++i) {
		map = g->s_inode_map[i] = s->s_inode_map[i];
		g->s_nfree_chunks += hweight8(~map);
	}
	g->s_nfree_chunks -= hweight8(~map);


	g->s_nfree_chunks += hweight8((~map)<<(8-g->s_nchunks%8)); /* ??  Buggy ??*/

	g->s_chunk_size = PAGE_SIZE/512;


#undef S
#undef _S	



	mutex_unlock(&g->s_mutex);
	return 0;
}
static int gfs_fill_super_info(struct gfs_super_info *si, u32 ino)
{
	struct gfs_inode_info *inode;
	struct gfs_super *raw;
	struct buffer_head *bh;
	int err;
	mutex_init(&si->s_mutex);
	
	inode = gfs_iget_info(si->s_vfs_sb, ino);
	PDEBUG("ino=%d PTR_ERR=%li\n", (int) ino, (long)PTR_ERR(inode));
	if (IS_ERR(inode)) 
		return PTR_ERR(inode);
	
	si->s_super_inode = inode;
	bh = get_inode_data(inode,0);
	if(!bh)
		return -EIO;
	MARK();	
	raw = (struct gfs_super *) (bh->b_data + sizeof(struct gfs_inode));
	
	err = init_super_from_raw(si, raw);

	put_inode_data(inode,0);
	return err;
}

static int gfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct gfs_super_info *si;
	struct inode  *inode;
	int r = -ENOMEM;
	
	if(!sb_set_blocksize(sb, 512))
		return -EFAULT;
	
	sb->s_magic = GFS_MAGIC+1;
	sb->s_op = &gfs_s_ops;

	si = kzalloc(sizeof(*si), GFP_KERNEL);
	if(!si)
		return -ENOMEM;
		
	sb->s_fs_info = si;
	si->s_vfs_sb  = sb;
	
	r = gfs_fill_super_info(si, 0);
	if(r)
		goto out;
	
	r = gfs_sysfs_init_sb(si);
	if(r)
		goto out;
	inode = gfs_iget(sb, si->s_ino_root);
	if(!inode)
		goto out_sysfs;

	si->s_root_inode = GFS_INODE(inode);
	
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		goto out_iput;


	return 0;
	
out_iput:
	iput(inode);
out_sysfs:
	gfs_sysfs_exit_sb(si);	
out:
  	kfree(si);
  	PVAR(r,"%d");
	return r;
}
void gfs_kill_super(struct super_block *sb)
{
	struct gfs_super_info *sbi = (struct gfs_super_info *)sb->s_fs_info;
	
	gfs_sysfs_exit_sb(sbi);
	kill_litter_super(sb);
}

static struct dentry *gfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
	return  mount_bdev(fs_type, flags, dev_name ,data, gfs_fill_super);
}
static struct file_system_type gfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "gfs2",
	.mount		= gfs_mount,
	.kill_sb	= gfs_kill_super,
};


static int __init gfs_init(void)
{
	//check_sizes();
	//gfs_inode_cache = kmem_cache_create("gfsinode_cache", sizeof(struct gfs_inode_info),
	//		0, (SLAB_RECLAIM_ACCOUNT|SLAB_POISON|SLAB_RED_ZONE|SLAB_PANIC),
	//		init_once);
	int r = register_filesystem(&gfs_type);
	if(r)
		goto ret;
	r = gfs_sysfs_init();
	if(r)	
		goto unreg;
	return 0;
unreg:		
	unregister_filesystem(&gfs_type);	
ret:
	PVAR(r, "%d");
	return r;
}

static void __exit gfs_exit(void)
{
	gfs_sysfs_exit();
	unregister_filesystem(&gfs_type);
}

module_init(gfs_init);
module_exit(gfs_exit);
