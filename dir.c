#include <linux/slab.h>
#include "gfs.h"

static void gfs_init_dir_info(struct gfs_inode_info *dir, struct gfs_dir_info *p, int write)
{
	struct gfs_super_info *sb = (struct gfs_super_info *)dir->vfs_inode.i_sb->s_fs_info;
	p->off  = -(int)sizeof(struct gfs_dir);
	p->block = 1;                           /* 0'th is left for the inode */
	p->write = write;
	p->dir = dir;
	p->blocksize = sb->s_vfs_sb->s_blocksize;
	p->chunksize = sb->s_chunk_size;

	p->valid = i_size_read(&dir->vfs_inode);
//	p->valid = dir->vfs_inode.i_size;	
	if(!get_inode_block(p->dir, p->block, p->write))
		return;
	p->addr = p->dir->i_bh->b_data;
}
/**
 * @return 1 if found something,
 *	   0 if nothing to find,
 *	  <0 if error.
 */
static int gfs_get_dir_info(struct gfs_dir_info *p, int ignore_valid)
{
	if(!p->valid && !ignore_valid)
		return 0;
	
	
	p->off += sizeof(struct gfs_dir);
	if(p->off > p->blocksize) {
		p->off = 0;
		p->block ++;
		put_inode_block(p->dir, p->write);
		
		if(p->block > p->chunksize) 
			return 0;
		
		if(!get_inode_block(p->dir, p->block, p->write))
			return -EIO;
		p->addr = p->dir->i_bh->b_data;
	}
		
	p->raw_dir = (struct gfs_dir *)(p->addr + p->off);
	if(p->raw_dir->d_ino) 
		p->valid --;
	
	return 1;

}
static struct gfs_dir *get_empty_dentry(struct gfs_inode_info *dir, int write)
{	
	struct gfs_dir_info d;
	gfs_init_dir_info(dir, &d, write);
	while(1) { 
		int r = gfs_get_dir_info(&d, 1);
		if(r <= 0 )
			return ERR_PTR(r);
		PDEBUG("ino: %i\n",d.raw_dir->d_ino);	
		if(d.raw_dir->d_ino == 0)	
			return d.raw_dir;
	
	}
	return NULL;
	
}
static int gfs_add_dir_entry(struct gfs_super_info *sb, struct gfs_inode_info *dir, char *name, u32 ino)
{	
	struct gfs_dir *p;
	int r = 0;
		

	p = get_empty_dentry(dir,1);
	if(IS_ERR_OR_NULL(p)) {
		r = PTR_ERR(p);
		goto put;
	}
	memcpy(p->d_name, name, GFS_FNAME);
	p->d_ino = cpu_to_le32(ino);
	i_size_write(&dir->vfs_inode, i_size_read(&dir->vfs_inode) + 1);

	mark_inode_dirty(VFS_INODE(dir));
	sync_inode_data(dir);
	
put:
	gfs_dbg("Adding '%s'(%ld) to %ld ret=%i\n", name, (long)ino, dir->vfs_inode.i_ino, r);	
	put_inode_data(dir, 1);
	return r;
}
static int gfs_del_dir_entry(struct gfs_super_info *sb, struct gfs_inode_info *dir, char *name)
{
	struct gfs_dir_info di;
	int err = -ENOENT;
	

	
	gfs_init_dir_info(dir, &di,1);
	
	while(gfs_get_dir_info(&di,0))
	
		if(strncmp(di.raw_dir->d_name, name, GFS_FNAME) == 0) {
			i_size_write(&dir->vfs_inode, i_size_read(&dir->vfs_inode) - 1);		
//				dir->vfs_inode.i_size --;
			memset(di.raw_dir, 0, sizeof(struct gfs_dir));
			err = 0;
			break;
		}
	
	gfs_dbg("Removing '%s'(%ld) from %ld ret=%i\n", 
		name, (long)di.raw_dir->d_ino, dir->vfs_inode.i_ino, err);	
	put_inode_data(dir, 1);
	
	return err;
}


static u32 gfs_find_by_name(struct gfs_inode_info *dir, char *name, int *err)
{
	struct gfs_dir_info di;
	int r;
	u32 ino;
	
	gfs_init_dir_info(dir, &di, 0);
	
	while(1) {
		switch(r = gfs_get_dir_info(&di,0)) {
		case 1 :
			 break;
		case 0 : 
			goto out;	
		default: 
			*err = r;
			goto out;
		} 
		if(strncmp(di.raw_dir->d_name, name, GFS_FNAME) == 0) {
			ino = di.raw_dir->d_ino;
			
			put_inode_data(dir, 0);
			return ino;
		}
	}
out:

	put_inode_data(dir,0);
	return 0;
}
static struct dentry *gfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	struct gfs_inode_info *de = GFS_INODE(dir);
//	struct dentry *ret = NULL;
	struct inode *inode = NULL;
	int err;
	u32 ino;

	gfs_dbg("looking up %s at %lu\n", (char *)dentry->d_name.name, dir->i_ino);

	ino = gfs_find_by_name( de, (char *)dentry->d_name.name, &err);
	if(ino) {
		inode = gfs_iget(dir->i_sb, ino);
		if(!inode) 
			return ERR_PTR(-EIO);
	}
	//ret = d_materialise_unique(dentry, inode);
	d_add(dentry, inode);	
	//ret = dentry;

//	PDEBUG("ret(%s)=\n",IS_ERR(ret) ? "err": "nonerr");
	
	return NULL;
	
	
}
static int gfs_mknod(struct inode *dir,struct dentry *dent,umode_t mode,dev_t dev)
{
	int r;
	struct inode *i = gfs_iget_new(dir->i_sb->s_fs_info);
	if(IS_ERR(i)) {
		PDEBUG("i get new\n");
		return PTR_ERR(i);
	}
	
	gfs_dbg("creating %s(ino=%ld,mode=0%o) in %ld\n", 
		dent->d_name.name, i->i_ino, mode, dir->i_ino);
	
	gfs_init_inode(i, dent->d_parent, (u16) mode);
	
	r = gfs_add_dir_entry(dir->i_sb->s_fs_info, GFS_INODE(dir),
			     (char*)dent->d_name.name, (u32)i->i_ino);
	if(r) {
		PDEBUG("add dir entry\n");
		return r;
	}


	d_instantiate(dent, i);
	
//	sync_inode_data(GFS_INODE(i));
	mark_inode_dirty(i);
	return r;
}
static int gfs_create(struct inode *dir,struct dentry *dent, 
		      umode_t mode, bool excl)
{
	return gfs_mknod(dir, dent, mode | S_IFREG, 0);
}
static int gfs_mkdir(struct inode *dir,struct dentry *dent,umode_t mode)
{
	return gfs_mknod(dir, dent, mode | S_IFDIR, 0);
}

static int gfs_unlink(struct inode *dir, struct dentry *dent)
{
	struct inode *inode = dent->d_inode;
	int err;
	lock_inode_data(GFS_INODE(inode), 1);

	err = gfs_del_dir_entry(dir->i_sb->s_fs_info ,GFS_INODE(dir), 
				(char*)dent->d_name.name);
	unlock_inode_data(GFS_INODE(inode), 1);
	
	if(err) 
		return err;
	
	//drop_nlink(inode);
	gfs_put_inode(GFS_INODE(inode));
	return 0;
}
static int gfs_rmdir(struct inode *dir, struct dentry *dent)
{
	if(dent->d_inode->i_size)
		return -ENOTEMPTY;
	return gfs_unlink(dir, dent);
}
struct inode_operations gfs_dir_inode_operations = {
	.create = gfs_create,
	.unlink = gfs_unlink,
	.mkdir = gfs_mkdir,
	.rmdir = gfs_rmdir,
	.lookup = gfs_lookup,
};
EXPORT_SYMBOL(gfs_dir_inode_operations);
/* 	Directory file operations */

static int gfs_opendir(struct inode *inode, struct file *file)
{
	struct gfs_dir_info *dir = kzalloc(sizeof(*dir),GFP_KERNEL);
	if(!dir)
		return -ENOMEM;
	gfs_init_dir_info(GFS_INODE(inode), dir, 0);
	file->private_data = dir;
	return 0;
}
static int gfs_release(struct inode *inode, struct file *file)
{
	
	kfree(file->private_data);
	return 0;
}

static int gfs_readdir(struct file * filp, void * dirent, filldir_t filldir)
{
	struct gfs_inode_info *e = GFS_INODE(filp->f_inode);
	int r = 0,  fdBreak = 0;
	ino_t ino = e->vfs_inode.i_ino;
	struct inode *inode;

	switch(filp->f_pos) {
	case 0:

		if (filldir(dirent, ".", 1, filp->f_pos, ino, DT_DIR) == 0)
			filp->f_pos++;
	
	case 1:

		if (filp->f_dentry->d_parent)
			ino = filp->f_dentry->d_parent->d_inode->i_ino;

		if (filldir(dirent, "..", 2, filp->f_pos, ino, DT_DIR) == 0)
			filp->f_pos++;

	case INT_MAX:
		return 0;
	default:
	{
		struct gfs_dir_info *dir = filp->private_data;	

		while(1) {
			r = gfs_get_dir_info(dir,0);
			if(r != 1 )
				goto out;
			
			if(dir->raw_dir->d_ino) {
	 			inode = gfs_iget( e->vfs_inode.i_sb, 
	 					  le32_to_cpu(dir->raw_dir->d_ino));
			
				if(inode) {
					if (filldir(dirent, dir->raw_dir->d_name, 
						    strlen(dir->raw_dir->d_name), 
						    filp->f_pos, dir->raw_dir->d_ino, 
						    ((inode)->i_mode >> 12) & 15)) {
						fdBreak = 1;
						goto fdBreak;
					}
				filp->f_pos++;    
				}
			}
		}
out:
		put_inode_data(e, 0);
	}
	}	
fdBreak:
	if ((filp->f_pos > 1) && !fdBreak) { /* EOF */
		filp->f_pos = INT_MAX;
	}

	return r;
}


struct file_operations gfs_dir_operations = {
	.read		= generic_read_dir,
	.readdir	= gfs_readdir,
	.open 		= gfs_opendir,
	.release	= gfs_release,
	.llseek		= generic_file_llseek,
};
EXPORT_SYMBOL(gfs_dir_operations);
