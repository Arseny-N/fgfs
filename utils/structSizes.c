#include <stdio.h>
#include <linux/types.h>

#include "structs.h"

int main( int argc, char **argv)
{
#define P(s, ex)	printf("\"%s\" expected size = %d current size = %d\n",#s,ex, sizeof(s))
	P(struct gfs_super, RAW_SUPER_SIZE);
	P(struct gfs_inode, RAW_INODE_SIZE);
	P(struct gfs_dir, RAW_DIR_SIZE);
#undef P
	return 0;
}
