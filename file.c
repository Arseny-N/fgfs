
#include "gfs.h"
static int gfs_get_block(struct inode *inode, sector_t block, struct buffer_head *bh, int create)
{
	int ch_size = get_chunk_size(GFS_INODE(inode));;
	long off = (inode->i_ino*ch_size)+block;
	
	PDEBUG("inode=%ld, block=%lu, create=%d, off=%ld\n",(long) inode->i_ino, ++block, create, off);
	if(block >= ch_size)             /* TODO: allocate a new inode, mark it as extended & use it. */
		return -EIO;	


	map_bh(bh, inode->i_sb, off);

#define PBOOL(s, b) PDEBUG(s ": %s\n", b ? "yes" : "no")
/*
	PBOOL("New      ", buffer_new(bh));
	PBOOL("Mapped   ", buffer_mapped(bh));
	PBOOL("Locked   ", buffer_locked(bh));
	PBOOL("Unwritten", buffer_unwritten(bh));
	PBOOL("Delay    ", buffer_unwritten(bh));
	PBOOL("Uptodate ", buffer_uptodate(bh));
	PBOOL("Page uptodate", PageUptodate(bh->b_page));
	PDEBUG("Address  : %p\n", bh->b_data);
*/
	return 0;
	
}
static int gfs_writepage(struct page *page, struct writeback_control *wbc)
{
	return block_write_full_page(page, gfs_get_block, wbc);
}

static int gfs_readpage(struct file *file, struct page *page)
{

	return block_read_full_page(page, gfs_get_block);
}


static void gfs_write_failed(struct address_space *mapping, loff_t to)
{
	struct inode *inode = mapping->host;

	if (to > inode->i_size) {
		truncate_pagecache(inode, to, inode->i_size);
		//gfs_truncate(inode);
	}
}

static int gfs_write_begin(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned flags,
			struct page **pagep, void **fsdata)
{
	int ret;

	ret = block_write_begin(mapping, pos, len, flags, pagep,
				 gfs_get_block);
	if (unlikely(ret)) 
		gfs_write_failed(mapping, pos + len);        /* TODO: what should we do here? */
	

	return ret;
}
static int gfs_write_end(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata)
{
	return generic_write_end(file, mapping, pos, len, copied, page, fsdata);

}
static sector_t gfs_bmap(struct address_space *mapping, sector_t block)
{
	return generic_block_bmap(mapping,block,gfs_get_block);
}






struct address_space_operations gfs_aops = {
	.readpage = gfs_readpage,
	.writepage = gfs_writepage,
	.write_begin = gfs_write_begin,
	.write_end = gfs_write_end,
	.bmap = gfs_bmap
};

EXPORT_SYMBOL(gfs_aops);



struct file_operations gfs_file_operations = {

	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.write		= do_sync_write,
	.aio_read	= generic_file_aio_read,
	.aio_write	= generic_file_aio_write,
};
EXPORT_SYMBOL(gfs_file_operations);

