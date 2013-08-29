#ifndef __GFS_STRUCTS_HDR__
#define __GFS_STRUCTS_HDR__

#define RAW_INODE_SIZE 24
#define RAW_SUPER_SIZE 12
#define RAW_DIR_SIZE   20

#define GI_INO_EXT  0x1
#define GI_INO_USED 0x2

#define GFS_FNAME 15

#ifndef BUILD_BUG_ON
# define BUILD_BUG_ON(a)
#endif

#define CHECK_SIZE(s,size) 	        	\
static inline void check_##s(void) 		\
{						\
	BUILD_BUG_ON(sizeof(struct s) != size);	\
}


struct gfs_inode {
	__le32 i_size;
	__le32 i_time;
	__le32 i_exino;      
	__le16 i_flags;	/* GI_INO_* flags */
	__le16 i_mode;
	__le32 i_id;
	__le32 i_nlink;
};
CHECK_SIZE(gfs_inode, RAW_INODE_SIZE);


struct gfs_dir {
	__u8   d_name[GFS_FNAME];
	__le32 d_ino;                  /* If ino > 0 then used */
};
CHECK_SIZE(gfs_dir, RAW_DIR_SIZE);


struct gfs_super {	
	__le32 s_nchunks;
	__le32 s_ino_root;
	__u8   s_chunk_size;
	__u8   s_inode_map[];
};
CHECK_SIZE(gfs_super, RAW_SUPER_SIZE);



static inline int __is_bit_used(__u8 *map, __u32 off)
{
	return map[off/8] & (0x1 << (off%8));
}
static inline void __mark_bit_as_used(__u8 *map, __u32 off)
{
	map[off/8] |= 1 << off%8;
}
static inline void __mark_bit_as_unused(__u8 *map, __u32 off)
{
	map[off/8] &= ~(1 << off%8);
}


static inline int first_bit(__u8 b, __u8 bit)
{
	if(!bit)
		b = ~b;
#define S(n) (b & (1<<n)) ? (n)
	return S(7) : S(6) : S(5) : S(4) : 
	       S(3) : S(2) : S(1) : S(0) : 0;
#undef S
}

static inline int first_set_bit(__u8 b)
{
	return first_bit(b, 1);
}

static inline int first_zero_bit(__u8 b)
{
	return first_bit(b, 0);
}
static inline int __count_map_size(int nchunks)
{
	/* TODO: DIV_ROUND_UP in Glibc? */
	return (nchunks + 8 - 1) /8;
}


#endif
