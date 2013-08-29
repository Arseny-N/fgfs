
#include <linux/sysfs.h>

#include "gfs.h"

struct gfs_attr {
	struct attribute attr;
	ssize_t (*show)(struct gfs_super_info *,struct gfs_attr *, char *);
};

#define DECLARE_GFS_SI_SHOW(elem, name, fmt) \
static ssize_t name##_show(struct gfs_super_info *si,struct gfs_attr *a, char *buf)\
{\
	return snprintf(buf, PAGE_SIZE, fmt, si->elem);\
}\
static struct gfs_attr attr_##name = __ATTR_RO(name);

DECLARE_GFS_SI_SHOW(s_chunk_size,chunk_size, "%u\n");
DECLARE_GFS_SI_SHOW(s_blocksize,block_size, "%u\n");
DECLARE_GFS_SI_SHOW(s_nchunks,chunks, "%u\n");
DECLARE_GFS_SI_SHOW(s_nfree_chunks,free_chunks, "%u\n");
DECLARE_GFS_SI_SHOW(s_ino_root,root_inode, "%u\n");

static ssize_t imap_show(struct gfs_super_info *si,struct gfs_attr *a, char *buf)
{
	int i = 0,map_size = __count_map_size(si->s_nchunks);
	size_t len = 0;
	
	for(i=0 ; i < map_size; ++i)  
		len += snprintf(buf + len, PAGE_SIZE - len, "%.2x ", si->s_inode_map[i]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	
	return len;
}
static struct gfs_attr attr_imap = __ATTR_RO(imap);

#define __A_LIST(a) &(attr_##a.attr)
static struct attribute *gfs_attrs[] = {
	__A_LIST(imap),
	__A_LIST(root_inode),
	__A_LIST(chunk_size),
	__A_LIST(block_size), 
	__A_LIST(chunks),
	__A_LIST(free_chunks),
	NULL,
};


static ssize_t gfs_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct gfs_super_info *sb = container_of(kobj, struct gfs_super_info, s_kobj);
	struct gfs_attr *a = container_of(attr, struct gfs_attr, attr);
	return a->show ? a->show(sb, a, buf) : 0;
}
static const struct sysfs_ops gfs_attr_ops = {
	.show   = gfs_attr_show,
};
static struct kobj_type gfs_ktype = {
        .default_attrs  = gfs_attrs,
        .sysfs_ops      = &gfs_attr_ops,
//      .release        = gfs_sb_release,
};
static struct kset *gfs_kset = NULL;

int gfs_sysfs_init_sb(struct gfs_super_info *sbi)
{
	struct super_block *sb = sbi->s_vfs_sb;
	int err = 0;
	BUG_ON(!gfs_kset);
	sbi->s_kobj.kset = gfs_kset;
	err = kobject_init_and_add(&sbi->s_kobj, &gfs_ktype, NULL, "%s", sb->s_id);

	return err;
}
EXPORT_SYMBOL(gfs_sysfs_init_sb);
void gfs_sysfs_exit_sb(struct gfs_super_info *sbi)
{
	kobject_del(&sbi->s_kobj);
}
EXPORT_SYMBOL(gfs_sysfs_exit_sb);
int gfs_sysfs_init(void)
{
	int err = 0;
	gfs_kset = kset_create_and_add("gfs", NULL, fs_kobj);
	if (!gfs_kset) 
                err = -ENOMEM;
	return err;
}
EXPORT_SYMBOL(gfs_sysfs_init);
void gfs_sysfs_exit(void)
{
	kset_unregister(gfs_kset);
}
EXPORT_SYMBOL(gfs_sysfs_exit);

