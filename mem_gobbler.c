#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h> // needed for getrusage 
/*
 * Memory Gobbler.
 *
 *
 * Basic idea: randomly allocate either anon or file pages
 *
 * * randomizer:
 * * * select the time of holding time:
 * * * init new phase
 * * * select the count of the allocation in transition phase:
 * * * * randomize this count between anon and
 * * * * file mapped pages based on user input ratio.
 * * * * * * within the holding time, randomly touch im-previous phase 				
 * * * * * * * Repeat this till all the files in all list are exhausted.
 *
 * Anon and file mapped pages: Maintain 3:1 ratio
 */

/*
 * TODO: 
 *	Use both the kind of anon pages.
 *	get opt
 * 		phase max time
 */
typedef unsigned long lt_t;
#define NUMS 4096
#define MAX_ALLOC (4096)
#define ms2ns(ms) ((ms)*1000000LL) 

/*
 * Concatenator 
 */
#define FS "/mnt/test_images/"
#define PATH(name) FS # name

/* This defines maximum allocation in an transition
 *
 */
#define MAX_TRANSITION_CNT 24
#define MAX_TRANS_HALF (MAX_TRANSITION_CNT/2)

static int num[NUMS];
static char* progname;

/*
 * Statistics: 
 *	counting Anon and Filemapped pages
 */
static lt_t anon_cnt = 0;
static lt_t file_cnt = 0;
static lt_t phase_cnt = 0;
struct rusage res;

/*
 * Basically tries to maintain
 * array of list of paths and its
 * access.
 *
 */
typedef struct _file_path {
	int alloc;
	char *path;
}file_path;

typedef struct file_lst_size {
	lt_t size;
	file_path *paths;
} file_lst_size_t;

typedef enum mem_type {
	file = 0,
	anon,
	type_max
}mem_type_t;

typedef struct alloc_list_per_se {
	lt_t *addr;	/* starting address */
	lt_t len;	/* length of the mmap*/
	mem_type_t type; /* type of allocation*/
}alloc_list_per_se_t;

/* Per phase allocation list */
typedef struct alloc_list {
	lt_t list_count;
	alloc_list_per_se_t lst[MAX_ALLOC];
}alloc_list_t;

/* List of phases */
signed long curr = -1; /*current phase index*/
static alloc_list_t alloc_track[MAX_ALLOC];

#include "files.h"

static inline void print_paths(file_lst_size_t *gpath)
{
	file_path *path = gpath[0].paths;

	while(path->path) {
		printf("%s\n", path->path);
		path++;
	}
}

static file_path *find_avail_node(int i)
{
	file_path *node = file_lst[i].paths;
	while(node->path) {
		if(!node->alloc) {
			break;	
		}
		node++;
	}

	if(!node->path)
		return NULL;
	return node;
}

static file_path* get_file_path(unsigned int len)
{
	file_path *node = NULL;
	int i;

	for(i = 0; i<max; i++) {
		//printf("%d\n", len);
		if(file_lst[i].size >= len) {
			node = find_avail_node(i);
			if(!node)
				continue;
		}
	}
	return node;
}

/* Allocation tracker */
/*
 *
 * This is the explicit allocated pool,
 * Test job will randomly fetch from this list 
 * and touches the corresponding alloc randomly.
 *
 */
static void go_to_nxt_phase()
{
	if(curr >= MAX_ALLOC) {
		printf("reached maximum pahse transitions\n");
		exit(1);
		return;
	}
	curr++;
}

static void alloc_track_init()
{
	memset(&alloc_track, 0x00, sizeof(alloc_track));
}

static int add_new_alloc(lt_t *address, long len, mem_type_t type)
{
	lt_t i = alloc_track[curr].list_count;

	if(i >= MAX_ALLOC) {
		//printf("Curr: %ld MAX alloc : %ld\n", curr, i);
		return -1;
	}
	alloc_track[curr].lst[i].addr = address;
	alloc_track[curr].lst[i].len = len;
	alloc_track[curr].lst[i].type = type;
	alloc_track[curr].list_count++;
	return 0;
}

#if 0
/* XXX: I dont want to use this :D*/
static int del_alloc(long  phase, lt_t *address)
{
	int i = 0;
	int found = 0;

	if(phase > curr) {
		fprintf(stderr, "wrong phase alloc\n");
		exit(1);
	} 
	while(i < alloc_track[phase].list_count) {
		if(alloc_track[phase].lst[i].addr == address) {
			found = 1;
			i++;
			break;
		}
		i++;
	}
	while(i < alloc_track[phase].list_count) {
		alloc_track[phase].lst[i-1].addr = alloc_track[phase].lst[i].addr;
		alloc_track[phase].lst[i-1].len =  alloc_track[phase].lst[i].len;
		alloc_track[phase].lst[i-1].type = alloc_track[phase].lst[i].type;
		i++;
	}

	if(found) {
		alloc_track[phase].list_count--;
		return 0;
	}

	return -1;
}
#endif 

/* randomizer */
/* 
 * A module that randomizes a number within given range
 *	range: 0 - limit ( excluded )
 *
 * Taken from: stack overflow.
 */
static long rand_lim(long limit) 
{
	long divisor = RAND_MAX/(limit+1);
	long retval;

	do {
		retval = rand() / divisor;
    	} while (retval >= limit);

	return retval;
}

/// Begin and end are *inclusive*; => [begin, end]
lt_t rand_intr(lt_t begin, lt_t end) {

    lt_t range = (end - begin) + 1;
    lt_t limit = (RAND_MAX) - ((RAND_MAX) % range);

    /* Imagine range-sized buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    lt_t randVal = rand();
    while (randVal >= limit) randVal = rand();

    /// Return the position you hit in the bucket + begin as random number
    return (randVal % range) + begin;
}

/* 
	Thought is to maintain 3:1 ratio between anon and file
	Slice defines that ratio: if anon_slice is 0 then only possibility
	is file anon.
*/
static int randomize_alloc_type(int anon_slice)
{
	if(anon_slice)
		return rand_lim(type_max);
	return file;
}

static lt_t randomize_alloc_len()
{
	int i = rand_lim(max);

	return file_lst[i].size;
}

static int randomize_alloc_count()
{
	return rand_lim(MAX_TRANSITION_CNT);
}

static void random_touch(lt_t *addr, lt_t begin, lt_t end, lt_t len)
{
	#define MAX_TRY 20
	int i = 0;

	while(i < MAX_TRY) {
		lt_t i = rand_intr(begin, end-10);
		if(i >= len)
			continue;
		// TODO: getrusage
		if(addr[i] != 0xf) {
			//printf("touchi\n"); 
			addr[i] = 0xf;
			break;
		}
		i--;
	}
}

static void random_touch_n(lt_t *addr, lt_t len)
{
	lt_t which_slice = 0;
	lt_t switcher = len/2;
	lt_t n = len/1024;
	//printf("len: %ld random: %ld\n",len, n);
	while(n) {
		lt_t start = which_slice? switcher : 0;
		lt_t end = which_slice? len : switcher;	
		random_touch(addr, start, end, len);
		which_slice = !which_slice;
		n--;
	}
}
/*Allocator*/
/*
 * This randomizes the sequence
 * of allocation. 
 *
 */
#define PAGE_KB 4096
#define BYTE_TO_PAGE(B) ((B) / (PAGE_KB))

lt_t* mmapper(char *path, lt_t size, mem_type_t type)
{
	int fd;
	lt_t *map;
	lt_t page_cnt = BYTE_TO_PAGE(size);

	if(anon == type) {
		map = mmap(NULL, size,
			PROT_READ | PROT_WRITE, 
			MAP_SHARED| MAP_ANONYMOUS, -1, 0);
		anon_cnt += page_cnt;	
	} else if (file == type) {
		if(!path) {
			fprintf(stderr, "path is null\n");
			exit(1);
		}
		fd = open(path, O_RDWR);
		if(-1 == fd) {
			perror("open");
			exit(1);
		}
		map = mmap(NULL, size,
			 PROT_READ | PROT_WRITE,
			 MAP_PRIVATE, fd, 0);
		file_cnt += page_cnt;
	}
	if(MAP_FAILED == map) {
		//printf("%s, %d, type: %d\n",path, size, type);
		perror("mmap");
		exit(1);
	}

	if (madvise(map, size, MADV_RANDOM)) {
		perror("madvise");
		exit(1);
	}

	return map;
}

lt_t* alloc(mem_type_t type, unsigned int len, file_path **node)
{
	//printf("type: %d len: %d\n", type, len);
	if(anon == type) {
		*node = NULL;
	}
	else {
		*node = get_file_path(len);
	}
	return mmapper(( (*node) ? (*node)->path : NULL),len, type); 
}

static mem_type_t random_allocator_one( int anon_slice )
{
	int alloc_type = randomize_alloc_type(anon_slice);
	lt_t alloc_len = randomize_alloc_len();
	lt_t *addr;
	file_path *node = NULL;
	int alloc_suc;

	addr = alloc(alloc_type, alloc_len, &node);
	alloc_suc = add_new_alloc(addr, alloc_len, alloc_type);

	if(node && (alloc_suc >=0))
		node->alloc = alloc_suc;
	else if(alloc_suc < 0) {
		fprintf(stderr,"Failed to add new tracker");
		exit(1);
	}
	return alloc_type;
}

/* CPU time consumed so far in seconds */
double cputime(void)
{
	struct timespec ts;
	int err;
	err = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
	if (err != 0)
		perror("clock_gettime");
	return (ts.tv_sec + 1E-9 * ts.tv_nsec);
}

/* wall-clock time in seconds */
double wctime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

int lt_sleep(lt_t timeout)
{
	struct timespec delay;

	delay.tv_sec  = timeout / 1000000000L;
	delay.tv_nsec = timeout % 1000000000L;
	return nanosleep(&delay, NULL);
}

static void touch(lt_t i)
{
	lt_t *addr;
	lt_t len;

	if(curr >  0) {
		if(i > 0) {
			//printf("random touch\n");
			addr = alloc_track[curr].lst[i].addr;
			len = alloc_track[curr].lst[i].len;
			random_touch_n(addr, len);
		}
	}
}

static int loop_once(void)
{
	int i, j = 0;
	for (i = 0; i < NUMS; i++)
		j += num[i]++;
	return j;
}

static void loop_n(int n)
{
	while(n) {
		lt_t cnt = alloc_track[curr].list_count;
		/* touch only last element*/
		touch(cnt-1);
		loop_once();
		n--;
	}
}

static int loop_for(double exec_time, double emergency_exit)
{
	double last_loop = 0, loop_start;
	int tmp = 0;
	double start = cputime();
	double now = cputime();
	long i = alloc_track[curr].list_count;

	while (now + last_loop < start + exec_time) {
		loop_start = now;

		/* touch the index i @ curr phase */
		if(--i >= 0) {
			touch(i);
		}
		else {
		   i = alloc_track[curr].list_count;
		}
		tmp += loop_once();
		now = cputime();
		last_loop = now - loop_start;
		if (emergency_exit && wctime() > emergency_exit) {
			fprintf(stderr, "oops!!!: memspin/%d emergency exit!\n", getpid());
			fprintf(stderr, "Something is seriously wrong! Do not ignore this.\n");
			break;
		}
	}
	return tmp;
}

static void trans_rand_alloc()
{
	#define MAX_LOOP 3
	int cnt = 0;
	int i = rand_lim(MAX_LOOP);
	int anon_slice;

	go_to_nxt_phase();

	while(cnt <= MAX_TRANS_HALF) {
		cnt = randomize_alloc_count();
	}
	anon_slice = cnt/4;
	//printf("anon slice %d\n", anon_slice);
	while(cnt) {
		mem_type_t typ;
		loop_n(i); /* Small loop to give reality*/
		typ = random_allocator_one(anon_slice);
		if(typ > file) {
			anon_slice--;
		}
		cnt--;
	}
}

static int job(double exec_time, double program_end, double length)
{
	double chunk1, chunk2;

	if (wctime() > program_end)
		return 0;
	else {
		trans_rand_alloc();

		chunk1 = drand48() * (exec_time - length);
		chunk2 = exec_time - length - chunk1;
		loop_for(chunk1, program_end + 1);

		/*TODO: Touch previous phase randomly*/
		//printf("%d[%ld]->%d\n", randomize_alloc_type(1),
		//			 randomize_alloc_len(),
		//		randomize_alloc_count());
		/* This is the place for randomn touch access */
		loop_for(length, program_end + 1);

		loop_for(chunk2, program_end + 2);
		phase_cnt++;
		//fprintf(stdout, "[US] %ld, F:%ld, A:%ld\n", phase_cnt, file_cnt, anon_cnt);
		return 1;
	}
}
__attribute__((destructor)) void on_process_exit()
{
	printf("Calling After exit call\n");
	getrusage(RUSAGE_SELF, &res);
	printf("%d, %ld, %ld, %ld, %ld\n", getpid(), file_cnt, anon_cnt, res.ru_minflt, res.ru_majflt);	
}

int main(int argc, char** argv)
{
	//lt_t wcet;
	//lt_t period;
	double wcet_ms, period_ms;
	double duration = 0, start = 0;
	double scale = 1.0;
	int cur_job = 0, num_jobs = 0;
///XXX: Command line args are needed, temporary removal is made...
	int verbose = 1;
	//unsigned int job_no;

	//int protocol = -1;
	double cs_length = 1; /* millisecond */

	progname = argv[0];
	wcet_ms   = 50; //atof(argv[optind + 0]);
	period_ms = 55; //atof(argv[optind + 1]);

	//wcet   = ms2ns(wcet_ms);
	//period = ms2ns(period_ms);

	duration  = 1000; //atof(argv[optind + 2]);
	duration += period_ms * 0.001 * (num_jobs - 1);
	start = wctime();

	alloc_track_init();

	printf("FORMAT:\n");
	printf("<PID>, <PHASE CNT>,  <DURATION>,  <FILEMAP : ANONMAP>, < MINFAULT : MAJFAULT>, <RSS>\n");
	do {
		getrusage(RUSAGE_SELF, &res);
		if (verbose) {
			printf("%d, %d,  %.4fms, %ld : %ld, %ld : %ld, %ld\n", 
				getpid(),
				cur_job,
				(wctime() - start) * 1000,
				file_cnt, anon_cnt,
				res.ru_minflt, res.ru_majflt,
				(res.ru_maxrss/4));
		}

		cur_job++;
		/* convert to seconds and scale */
	} while (job(wcet_ms * 0.001 * scale, start + duration,
		    cs_length * 0.001));

	return 0;
}

