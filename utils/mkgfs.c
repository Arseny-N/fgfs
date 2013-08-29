#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <linux/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../structs.h"
#if __BYTE_ORDER != __LITTLE_ENDIAN
#warning BIG_ENDIAN
static inline uint32_t tofs32(uint32_t i)
{
	return __bswap_32(i);
	//return i;
	
}
static inline uint16_t tofs16(uint16_t i)
{
	return __bswap_16(i);
	//return i;
	
}
static inline uint32_t toCpu(uint32_t i)
{
	return __bswap_32(i);
}
#else
static inline uint32_t tofs32(uint32_t i)
{
	return i;
	
}
static inline uint16_t tofs16(uint16_t i)
{
	return i;
	
}
static inline uint32_t toCpu(uint32_t i)
{
	return (i);
}
#endif
#define fatal(s)  do{perror(s), exit(EXIT_FAILURE);}while(0);
struct conf {
	char *fname;
	size_t block_size;
	int nblocks;
	
	int fd;
	void *block_buf;
	void *data;
	int numRead;
	
	bool dump;
};

static void prase_cmdLine(struct conf *c, char **argv, int argc)
{
	int i = 1;
	c->block_size = 4*1024;
	c->nblocks = 50;
	c->fname = "gfsRoot.img";
	c->dump = 0;
	
	while(i < argc) {			
		if(argv[i][0] == '-')
			switch(argv[i][1]) {
				case 'f':
					c->fname = argv[i][2] ? &argv[i][2] :argv [++i];
					break;
				case 'b':
					c->block_size = (size_t)strtoll(
								argv[i][2] ? &argv[i][2] :
								argv [++i],NULL, 0);
					break;
				case 'n':
					c->nblocks = (size_t)strtoll(
								argv[i][2] ? &argv[i][2] :
								argv [++i],NULL, 0);
					break;
				case 'd':
					c->dump = 1;
					break;
				default:
					fprintf(stderr, "%s: Bad cmd line argument - `%s', exiting.", 
											argv[0],argv[i]);
					exit(EXIT_FAILURE);
			}
		++i;
	}
}
static void init_conf(struct conf *cnf)
{
	cnf->block_buf = malloc(cnf->block_size);
	if(!cnf->block_buf)	
		fatal("malloc");
	
	cnf->fd = open(cnf->fname, O_RDWR | O_CREAT, 0777);
	if(cnf->fd == -1)
		fatal("open")
}
static size_t __write_inode(struct conf *conf)
{
	//printf("fd=%d, block_buf=%p, block_size=%d\n",conf->fd, conf->block_buf, conf->block_size);
	return write(conf->fd, conf->block_buf, conf->block_size);
}
static void init_inode_buf(struct conf *conf, int mode, int size)
{
	struct gfs_inode *i = (struct gfs_inode *) conf->block_buf;
	memset(conf->block_buf, 0, conf->block_size);
	
	conf->data = conf->block_buf + sizeof (struct gfs_inode);
	
#define SET(s,n) i->i_##s = tofs##n(s)
	SET(size, 32);
	SET(mode, 16);
//	SET(nlink, 16);	
#undef SET

}
static int write_inode(struct conf *conf, int inode, void *data, int size,int mode)
{
	int i;
	init_inode_buf(conf, mode, size);
	memcpy(conf->data, data, size);
	if(inode ==0 || inode == 1) {
		//for(i=0; i < conf->block_size; i++)
		//	printf("%x ", ((char *) conf->block_buf)[i]);
		struct gfs_inode *in = (struct gfs_inode *)conf->block_buf;
//		printf("size=%d,mode=%o\n",in->i_size, in->i_mode);
		if(!inode) {
			struct gfs_super *si = (struct gfs_super *) conf->data;
//			printf("root ino=%d,nblocks=%d\n", si->s_ino_root, si->s_blocks);
		}
	}
	__write_inode(conf);
}
static int init_super(struct conf *conf, struct gfs_super **super)
{
	int mapSize = __count_map_size(conf->nblocks);
	(*super) = malloc(sizeof(struct gfs_super) + mapSize);
	if(!*super) 
		fatal("malloc");
	
	memset(*super, 0, sizeof(struct gfs_super)+mapSize);
//	printf("Super (%d)\n", mapSize);
	(*super)->s_nchunks = conf->nblocks;

	(*super)->s_ino_root = tofs32(1);
	
	__mark_bit_as_used((*super)->s_inode_map, 0);
	__mark_bit_as_used((*super)->s_inode_map, 1);
	
	return sizeof(*super) + mapSize;
}
int main ( int argc, char * argv [] )
{
	struct gfs_super *super;
	struct conf conf;
	int super_len, i;
	
	prase_cmdLine(&conf, argv, argc);
	init_conf(&conf);	
	super_len = init_super(&conf, &super);
	
	write_inode(&conf, 0, super, super_len, S_IFREG | 0777);
	write_inode(&conf, 1, NULL, 0, S_IFDIR | 0777);
	for( i = 0;i < conf.nblocks; i++)
		write_inode(&conf, i+2, NULL, 0, 0);
		
	printf("Created fs in file %s.\n%ld blocks with bs %d.\nTotal size %ld bytes.\n",
			conf.fname, conf.nblocks, conf.block_size, conf.nblocks*conf.block_size);	
	return 0;
}




