#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <linux/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static char *fname = NULL, *progName = NULL;
static void prase_cmdLine(char **argv, int argc)
{
	int i = 1;
	
	progName = argv[0];
	fname = "gfsRoot.img";
	
	if(argv[1]) {
		while(i < argc) {			
			if(argv[i][0] == '-')
				switch(argv[i][1]) {
					case 'f':
						fname = argv[i][2] ? &argv[i][2] :argv [++i];
						break;
		
					default:
						fprintf(stderr, "%s: Bad cmd line argument - `%s', exiting.", 
												argv[0],argv[i]);
						exit(EXIT_FAILURE);
				}
			++i;
		}
	}
}
struct fuzzer_path {
	char *path;
	unsigned long flags;
	mode_t mode;
#define GET_RW(f) (f & FP_WRITE_BUF ? O_WRONLY : 0)

#define FP_DIR       0x1
#define FP_WRITE_BUF 0x2

	struct fuzzer_path *next;
	
	struct fuzzer_path *dir_next;

};
struct fuzzer_path_head {
	struct fuzzer_path *head;
	struct fuzzer_path *tail;
	
	struct fuzzer_path *dir_head;
	struct fuzzer_path *dir_tail;
	
	mode_t mode;
	char *buf;
};
struct fuzzer_timer {
	struct timeval tv;
	unsigned long flg;
};
static void __attribute__ ((noreturn)) __fatal(const char *func, unsigned long line, const char *cause, int err)
{
	fprintf(stderr, "%s failed in %s(%ld), failure cause - \"%s\" errno(%ld) is interpreted as %s\n", 
								progName, func, line,  cause,(long)errno, strerror(err));
	exit(EXIT_FAILURE);
}
static void __attribute__ ((noreturn)) __fatal_ne(const char *func, unsigned long line, const char *cause)
{
	fprintf(stderr, "%s failed in %s(%ld) failure cause - \"%s\"\n", progName, func, line,  cause);
	exit(EXIT_FAILURE);
}
#define fatal(err) __fatal(__func__, __LINE__, err, errno)
#define fatal_ne(err) __fatal_ne(__func__, __LINE__, err)

static char *strGet(char *str, char **pp, char sep)
{
	char *p;
	*pp = strchr(str, sep);
	if(*pp) 
		**pp = '\0';
		
	p = strdup(str);
	if(!p)
		fatal("strdup");
	if(*pp)
		**pp = sep;
	
	return p;
}
static int getDirs_recursive(char *path, char*ppath, struct fuzzer_path_head *ph)
{
	if(strcmp(path, ppath) == 0)
		return 0;
	char *base = basename(path);
	
	
}
static bool listed_in_dirs(char *name, struct fuzzer_path_head *ph)
{
	struct fuzzer_path *p;	
	for(p = ph->dir_head; p; p = p->dir_next) {	
		if(strcmp(name, p->name) == 0)
			return p;
	}
}
static struct fuzzer_path *find_entr(char *name, struct fuzzer_path_head *ph)
{
	struct fuzzer_path *p;	
	for(p = ph->head; p; p = p->next) {	
		if(strcmp(name, p->name) == 0)
			return p;
	}
	return NULL;
}
static void getDirs(char *path, struct fuzzer_path_head *ph)
{
	char *dirPath = dirname(path);
	char *base =   basename(dirPath);
	
	if(!listed_in_dirs(base, ph)) {
		struct fuzzer_path *p = find_entr(base, ph);
		if(!p)
			fatal_ne("Bad file\n");
	
		if( !S_ISDIR(p->mode) ){
			ph->dir_tail->dir_next = p;
			ph->dir_tail = ph->dir_tail->dir_next;
			
			p->mode &= ~S_IFREG;
			p->mode |= S_ISDIR;
		}
	}
}
static struct fuzzer_path *prase_line(char *line,struct fuzzer_path_head *ph)
{
	char *pp;
	struct fuzzer_path *p = malloc(sizeof(struct fuzzer_path));
	if(!p)
		fatal("malloc");

	p->path = strGet(line, &pp, ':');
	printf("path: %s\n", p->path);
	
	getDirs(p->path, ph)
	
	return p;
}
static void load_paths(char *file, struct fuzzer_path_head *p)
{
	struct fuzzer_path *fpth;
	char *line;
	FILE *fp = fopen(file, "r");
	if(!fp)
		fatal("fopen");
	p->head = p->tail = NULL;
	
	while(getline(&line, NULL, fp) != -1) {	
		fpth = prase_line(line, p);
		if(!p->head) {
			p->head = p->tail = fpth;
			continue;
		} 
		
		p->tail->next = fpth;
		p->tail = p->tail->next;
		
	}
	
	if(fclose(fp) != 0)
		fatal("fclose");
}
static struct fuzzer_timer *start_timer(void)
{
	return NULL;
}
static void stop_timer(struct fuzzer_timer *t)
{
	return;
}
#define MODE_MASK 0777
void create_paths(struct fuzzer_path_head *paths)
{
	struct fuzzer_path *p;
	for(p = paths->head; p; p = p->next ) {
		printf("Creating %s\n", p->path);
		switch(p->mode & S_IFMT) {
		case S_IFDIR:
			if(mkdir(p->path, p->mode & MODE_MASK ? : paths->mode & MODE_MASK))
				fatal("mkdir");
			break;
		case S_IFREG:
		default:
		{
			int fd = open(p->path, O_CREAT | GET_RW(p->flags), p->mode & MODE_MASK ? : paths->mode & MODE_MASK);
			if(fd == -1)
				fatal("open");
			if(GET_RW(p->flags)){
				/* Do something */
			}
	
			if(close(fd) == -1)
				fatal("close");
			break;
		}
		} /* Switch */
	}
}
int main ( int argc, char * argv [] )
{
	struct fuzzer_timer *timer;
	struct fuzzer_path_head paths;
	
	prase_cmdLine(argv, argc);
	load_paths(fname, &paths);

	timer = start_timer();	
		
	create_paths(&paths);
	
	stop_timer(timer);

	return 0;
}
