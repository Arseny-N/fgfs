#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linux/types.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#define __le32 uint32_t 
#define __le16 uint16_t
#define __u8 uint8_t

#define gfs_fatal(fmt, args...) do{fprintf(stderr, "%s :" fmt,strerror(errno), ##args); exit(EXIT_FAILURE); }while(0)

#include "structs.h"
struct file {
	unsigned long flags;
	
#define SET_INO     0x001
#define SET_SIZE    0x002
#define SET_TIME    0x004
#define SET_MODE    0x008
#define SET_ID      0x010
#define SET_NLINK   0x020
#define SET_NAME    0x040
#define SET_DIR_INO 0x080
#define SET_FLAGS   0x100
#define NO_FILE     0x200

	struct attrs {
		__u32 i_ino;
		__u32 i_size;
		__u32 i_time;
		__u32 i_exino;

		__u16 i_flags;	
		__u16 i_mode;
		__u16 i_id;
		__u16 i_nlink;
		
		__u32 i_dir_ino;
		char *name;
	}attrs;

	
	struct file_method {
		size_t (*get_data)(struct attrs *, void **buf);
		void (*put_data)(void *buf);
	} method;	
	
	struct file *next_set_ino;	
	struct file *next_set_dir_ino;
};

struct conf {
	char *fname;
	size_t block_size;
	int nblocks;
	

	int dev_fd;
	void *write_block_buf;
	int numRead;
	
	bool dump;
};
struct conf conf;


static size_t get_super_data(struct attrs *a, void **buf)
{	
	int mapSize = __count_map_size(conf.nblocks);
	struct gfs_super *super = malloc(sizeof(*super) + mapSize);
	if(!super) {
		gfs_fatal("malloc");
		return -1;
	}
	
	memset(super, 0, sizeof(*super)+mapSize);
	printf("Super (%d)\n", mapSize);
	super->s_blocks = conf.nblocks;

	super->s_ino_root = 1;
	*buf = super;
	__mark_inode_as_used(super->s_inode_map, 0);
	__mark_inode_as_used(super->s_inode_map, 1);
	
	return sizeof(*super) + mapSize;
}

static size_t get_root_data(struct attrs *a, void **buf)
{	
	printf("Root\n");
	*buf = NULL;
	return 0;
}
			
			
struct file 
	super = {
		.flags = SET_INO | SET_MODE | SET_FLAGS,
		.attrs =  { .i_mode = S_IFREG | 0777, .i_ino = 0, .i_flags = GI_INO_USED },
		.method = { .get_data = get_super_data,
			    .put_data = free 
			    } },
	root = {
		.flags = SET_INO | SET_MODE | SET_FLAGS,
		.attrs =  { .i_mode = S_IFDIR | 0777, .i_ino = 1, .i_flags = GI_INO_USED },
		.method = { .get_data = get_root_data, } },
	no_file = { .flags = NO_FILE};


struct file *files_array[] = { &super, &root,NULL};



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
	cnf->write_block_buf = malloc(cnf->block_size);
	if(!cnf->write_block_buf)	
		gfs_fatal("malloc");
	
	cnf->dev_fd = open(cnf->fname, O_RDWR| O_CREAT, 0777);
	if(cnf->dev_fd == -1)
		gfs_fatal("open");
}
static void write_block(struct conf *cnf, struct gfs_inode *ino, void *data, size_t len)
{
	int numWrote = 0, tmp;
	
	if(len > cnf->block_size)
		gfs_fatal("len > then dss!");

	memset(cnf->write_block_buf, 0, cnf->block_size - 1);	

	memcpy(cnf->write_block_buf, ino, sizeof(*ino));

	memcpy(cnf->write_block_buf + sizeof(*ino), data, len);

	do {
		tmp = write(cnf->dev_fd, cnf->write_block_buf + numWrote, cnf->block_size);
		if(tmp < 0) 
			gfs_fatal("write");
		
		numWrote += tmp; 
	} while( numWrote < cnf->block_size);

}

#if __BYTE_ORDER != __LITTLE_ENDIAN
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
static void mk_inode(struct file *f, struct gfs_inode *inode)
{
#define SET(s,n) inode->s = tofs##n(f->attrs.s)
	SET(i_flags, 32);
	SET(i_time, 16);
	SET(i_size, 32);
	SET(i_id, 16);
	SET(i_mode, 16);
	SET(i_nlink, 16);	
#undef SET
}
struct gfs_inode *read_block(struct conf *cnf)
{
	ssize_t numRead = 0, tmp;
	if(cnf->numRead >= cnf->block_size * cnf->nblocks )
		return NULL;
	do {
		tmp = read(cnf->dev_fd, cnf->write_block_buf+numRead,cnf->block_size);
//		printf("%d %d %d\n", numRead, tmp, cnf->dev_fd);
		if(tmp < 0){
			//printf("--%d %d\n", numRead, tmp);
			gfs_fatal("read");
		}
				
		numRead += tmp;
	} while(numRead < cnf->block_size);
	cnf->numRead += numRead;

	return (struct gfs_inode *) cnf->write_block_buf;
}
static void dumpDir(struct gfs_inode *i,struct conf *cnf)
{
	i+=1;
	struct gfs_dir *p = (struct gfs_dir *) i;
	
	while( (char*)p < (char*)(i+cnf->block_size)) {
		if(toCpu(p->d_flags) & DI_USED)
			printf(" %.*s->%ld|",16,p->d_name, toCpu(p->d_ino) );
		p++;
	}
	
}
static void dumpFs(struct conf *cnf)
{
	struct gfs_inode *inode;
	int ino = 0;
	printf(" ino  | type| mode| id  | size | time | cont \n");
	cnf->numRead = 0;
	while((inode = read_block(cnf))) {
		//if(toCpu(inode->i_flags) & GI_INO_USED) {
			printf("%5ld | %s | %.3o |%4d |%5d |%5d | %lx",
					ino++,
					S_ISREG(toCpu(inode->i_mode)) ? "reg" :
					S_ISDIR(toCpu(inode->i_mode)) ? "dir" : "???",
					toCpu(inode->i_mode) & 0777,
					toCpu(inode->i_id),
					toCpu(inode->i_size),
					toCpu(inode->i_time),
					toCpu(inode->i_flags));
			if(S_ISDIR(toCpu(inode->i_mode))) {
				dumpDir(inode, cnf);
			}
			printf("\n");
		//}
			
	}
}
int main ( int argc, char * argv [] )
{
	size_t i = 0, len; 
	
	struct gfs_inode inode;
	void *data;
	int fixedIno = 0, fixedDirIno = 0, cnt = 0;
	
	struct file *f = NULL, *first = NULL, *prev;	
	
	
//	check_sizes();

	prase_cmdLine(&conf, argv, argc);
	init_conf(&conf);
	
	if(conf.dump) {
		dumpFs(&conf);
		exit(EXIT_SUCCESS);
	}
	for(i = 0 ;files_array[i]; i++) {

		if(files_array[i]->flags & SET_INO) {
			fixedIno ++;
			if(f) 
				f->next_set_ino = files_array[i];
			else 
				first = files_array[i];
			f = files_array[i];
			f->next_set_ino = NULL;
		}
	}
	i = 0;

	while(i < conf.nblocks) {
		memset(&inode, 0, sizeof(inode));
		
		data = NULL;
		len = 0;
		f = NULL;
		
		if(fixedIno) {
			f = first;
			while(f) {
			
				if(f->attrs.i_ino == i) {
			
					if(f == first)
						first = f->next_set_ino;
					else
						prev->next_set_ino = f->next_set_ino;
					fixedIno --;
					break;
				}
				prev = f;
				f = f->next_set_ino;
			}
		}
		if(!f) {
			if(files_array[cnt]) {
				while(files_array[cnt]) {
					if(!(files_array[cnt]->flags & SET_INO))
						f = files_array[cnt];
				cnt ++;
				}
				
			}
			else {
				
				f = &no_file;
			}
		}
		
		if(f == &no_file) {
			printf("Nofile %p\n", );
		}
		
				printf("%d\n",i);			
		if(f->method.get_data ) {
				printf("%d\n",i);			
			len = f->method.get_data(&f->attrs, &data);
					printf("%d\n",i);			
		}
				printf("%d\n",i);			
		mk_inode(f, &inode);
		

		write_block(&conf,&inode, data, len);

		if(!(f->flags & NO_FILE) && f->method.put_data) 
			f->method.put_data(data);

		i++;						
	}
	if(!conf.dump)
		printf("Created fs in file %s.\n%ld blocks with bs %d.\nTotal size %ld bytes.\n",
			conf.fname, conf.nblocks, conf.block_size, conf.nblocks*conf.block_size);	
	exit(EXIT_SUCCESS);

}
