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
#include <stdbool.h>
#include <math.h>
#include <sys/resource.h> // needed for getrusage 
#include <unistd.h>

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

typedef unsigned long lt_t;

#define NUMS 4096
#define MAX_ALLOC (4096)
#define ms2ns(ms) ((ms)*1000000LL)

/* MAX number of pages*/ 
#define MAX_LIMIT 100000000
#define PAGE_SIZE 4096

/*
 * Concatenator 
 */
/*Enable this for ubuntu or other machines for verbose*/
#ifdef DEBUG
#undef DEBUG
#endif

#ifdef DEBUG
#define FS "/media/test_files/"
#else
#define FS "/media/test_images/"
#endif

#define PATH(name) FS # name

/* This defines maximum allocation in metric of pages in a transition
 *
 */
#define MAX_TRANSITION_CNT 150
#define MAX_TRANS_HALF (MAX_TRANSITION_CNT/2)

static char* progname;

/*
 * Statistics: 
 *	counting Anon and Filemapped pages
 */
static lt_t anon_cnt = 0;
static lt_t file_cnt = 0;
static lt_t phase_cnt = 0;
static lt_t alloc_nomore = 0;
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
	lt_t filled;
} file_lst_size_t;

typedef enum mem_type {
	file = 0,
	anon,
	type_max
}mem_type_t;

typedef struct alloc_list_per_se {
	char *addr;	/* starting address */
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
/* 
 * A set of data structures closely related to the  
 * automatic file generator.
 */
#include "files.h"

typedef enum {
	MEM_FIX = 0, /* Imitates Fix memory access */
	MEM_STRIDE, /* Imitates stride memory access */
	MEM_SEQ, /* Imitates sequential memory access */
	MEM_REP, /* Imitates repeat access */
	MEM_RAND, /* Imitates extreme pattern of random memory access */
	MEM_MAX
}mem_pattern_types;

/*
 * TUNING THE LIMITS:
 *	More precise the parameters becomes system 
 *	becomes more Deterministic.
 */

/* Maximum allocation possible within a phase*/
static lt_t max_alloc_per_phase = MAX_TRANSITION_CNT;
static unsigned char alloc_precision = false;
static lt_t max_limit = MAX_LIMIT; 		/* maximum allocation in page cnt */
static lt_t curr_alloc = 0; 			/* Tracking current allocatio ncount */
static lt_t speed = max-1; 			/* The value of max arrives from files.h*/
static lt_t access_type = MEM_RAND; 		/* Defines the memory access type */
static unsigned char in_transition = false; 	/* Tells the toucher that the
						 * system is in transition    */
static lt_t alloc_size = 0; 			/*if set, value is near precise during transition */

static lt_t total_alloc_pages=0;  		/*Tells, how many pages the gobbler really allocated */
static lt_t max_alloc_per_phase_byte = 0; 	/* Represent the max allocation/phase in bytes */
static lt_t vector = 0;				/* Vector of locality*/

/* ====================================Vectorization: Start============================ */

static long vector_init(char *path)
{

	return 0;
}

static long get_nxt_locality();
{
	return 0;
}

/* ====================================Vectorization: End==============================  */

/* ====================================randomizer: Start===============================  */

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
	unsigned int seed = (unsigned int)cputime();

	srand(seed);
	do {
		retval = rand() / divisor;
    	} while (retval >= limit);

	return retval;
}

/// Begin and end are *inclusive*; => [begin, end]
static lt_t rand_intr(lt_t begin, lt_t end) {

	lt_t range = (end - begin) + 1;
	lt_t limit = (RAND_MAX) - ((RAND_MAX) % range);
	lt_t upper = 128;
	unsigned int seed = (unsigned int)cputime();

	srand(seed);
	/* Imagine range-sized buckets all in a row, then fire randomly towards
	* the buckets until you land in one of them. All buckets are equally
	* likely. If you land off the end of the line of buckets, try again. */
	lt_t randVal = rand();
	while ((randVal >= limit) && upper) { 
		randVal = rand();
		upper--;
	}
	/// Return the position you hit in the bucket + begin as random number
	return (randVal % range) + begin;
}
#if 1 //testing

struct timespec timer_start(){
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    return start_time;
}

// call this function to end a timer, returning nanoseconds elapsed as a long
long timer_end(struct timespec start_time){
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = end_time.tv_nsec - start_time.tv_nsec;
    return diffInNanos;
}

#endif

static void touch_simple(char *addr)
{
	//struct timespec vartime = timer_start();
	//lt_t lapsed;		
	/* Notice the function reads and then writes*/
	if(*addr != 0xf) {
		*addr = 0xf;
	}
	else {
		*addr = 0x00;
	}
	//lapsed = timer_end(vartime);
	//printf(" access time: %ld\n", lapsed);
}

/* ====================================randomizer: End======================================= */


/*============================Access pattern Implementation: Start============================= */

/* Access pattern implementation:
 * From the paper: 	Online memory access pattern Analysis on an
 *			application profiling tool.
 * We can infer that there are 5 basic sequential memory access pattern:
 *	1. FIX: Fix represents a single memory access element that always refer the same address.
 *	2. STRIDE: access using a format defined.
 *	3. SEQUENTIAL: represents a single element of basic sequence of memory accesses without
 *			any strides or offsets.
 *	4. REPEAT: access to a particular element at a defined interval, imitates loop.
 *	5. RANDOM/COMPLEX: Other Extreme, completly sporadic. P.S. sporadicity depends on rand and rand_intr functionality.
 */

typedef void (*access_pattern)(char *addr, lt_t len);


/* 
 *	For FIX to work we need a static structure
 *	that book keeps the same element within 
 *	the addr range.
 *
 *	Following is a simple disjoint linear list that
 *	keeps track of the previous access point.
 *	TODO: Make it a self balancing tree eg: AVL.
 */
typedef struct __fix_bookeeper_node {
	char *addr;
	lt_t loc;
}fix_bk_node;

typedef struct __fix_bookeeper {
	fix_bk_node node;
	struct __fix_bookeeper *nxt;	
}fix_bookeeper;

fix_bookeeper *fix_head = NULL;

static fix_bk_node* fix_exists(char *const addr) 
{
	fix_bookeeper *iter = fix_head;

	while(iter) {
		if(addr == iter->node.addr) 
			break;
		iter = iter->nxt;
	}

	if(iter) {
		return &(iter->node);
	}
	return NULL;
}

static fix_bk_node* fix_alloc_node(char *addr)
{
	fix_bookeeper *b = malloc(sizeof(*b));
	if(b) {
		b->node.addr = addr;
		b->node.loc = 0;
		b->nxt = fix_head;
		fix_head = b;
	}
	return &(b->node);
}

static void fix_rand_loc(fix_bk_node *node, lt_t len)
{
	if(len <= 0)
		printf("len is less than zero\n");

	node->loc = rand_intr(0, len-10);
	if(node->loc > len) {
		printf("something wrong\n");
		node->loc = len - 100;
	}
	printf("deciding on loc : %ld\n", node->loc);
	node->addr[node->loc] = 0x00;	/* Simple touch */	
}

static void pattern_fix(char *addr1, lt_t len)
{
	char *addr = (char*)addr1;
	fix_bk_node *node;

	if(!addr || !len)
		goto fin;

	if(!(node = fix_exists(addr))) {
		if(node = fix_alloc_node(addr))
			fix_rand_loc(node, len);
		else {
			perror("Failure to alloc fix's node:");
			exit(EXIT_FAILURE);
		}
		goto fin;
	}
	touch_simple(&node->addr[node->loc]);
fin:
	return;
}


static lt_t stride_get_offset(lt_t len)
{

#define STRIDE_OFFSET(l, c) ((l)/(c))
#define STRIDE_CUT 1000

	lt_t stride = 0;
	lt_t piece = STRIDE_CUT;
 	
	do {
		stride = STRIDE_OFFSET(len, piece);
		piece = piece >> 1;		
	} while(!stride);

	return stride;
}

static void pattern_stride(char *addr1, lt_t len)
{
	char *addr = (char*)addr1;
	lt_t stride = stride_get_offset(len);
	lt_t i = 0;
	///XXX: handle user defined stride pattern.
	if(!stride) {
		fprintf(stderr, "stride returns 0\n");			
	}

	while(i < len) {
		touch_simple(&addr[i]);
		i = i + stride;
	}
	return;
}

static void pattern_seq(char *addr1, lt_t len)
{
	char *addr = (char*)addr1;
	lt_t i = 0;

	while(i < len) {
		touch_simple(&addr[i]);
		i++;
	}
	return;
}

/*
	To make the repeat work:
		1. we need a distance at which repeat should happen.
		2. How many times repeat should happen.
*/
static lt_t repeat_get_distance(const lt_t len)
{
	/*XXX: For now lets simply use stride get offset */
	lt_t distance =  stride_get_offset(len); 	
	return distance;
}

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))
#define CEILING_NEG(X) ((X-(int)(X)) < 0 ? (int)(X-1) : (int)(X))
#define CEIL(X) ( ((X) > 0) ? CEILING_POS(X) : CEILING_NEG(X) )

static lt_t repeat_get_count(const lt_t dist)
{
	#define MAX_DIST 1000
	#define MIN_CNT 2
	#define MAX_CNT 5
	/*
	 * If, repeat distance is big, then count should be small
	 * big? What do you mean by big?
	 * XXX: We have to define this more elobrately.
	 * but, for now we will hardcode it for now. 
	 */
	if(!dist)
		return 0;

	if(dist >= MAX_DIST) {
		return MIN_CNT;
	}
	else {
		double per_dec;

		per_dec = (MAX_DIST - dist);
		per_dec = per_dec/MAX_DIST;
		per_dec = CEIL((MAX_CNT * per_dec));

		return (per_dec);
	}
	return 0;
}

static void pattern_repeat (char *addr1, lt_t len)
{
	char *addr = (char*)addr1;
	lt_t dist = repeat_get_distance(len);
	lt_t rep_count;
	lt_t i;
	
	for(rep_count = repeat_get_count(dist); rep_count > 0; rep_count--) {
		i = 0;
		while(i < dist) {
			touch_simple(&addr[i]);
			i++;
		}
	}
	return;
}


static void random_touch_page(char *addr, lt_t len)
{
	#define MAX_ATT 25
	lt_t j = MAX_ATT;
	
	if(in_transition)
		j = 2;

	while(j) {
		lt_t index = rand_lim(len-20);
		pattern_seq(&addr[index], 10);
		j--;
	} 

}
static void random_touch(char *addr, lt_t begin, lt_t end, lt_t len)
{
	#define MAX_TRY 50
	lt_t j = MAX_TRY;
	lt_t i = 0;
	lt_t tmp_begin = begin;
	lt_t intr_len = end - begin;
	size_t page_size = sysconf(_SC_PAGE_SIZE);
	lt_t pages = intr_len/page_size;

	//printf("intr_len: %ld, so: %ld\n", intr_len, pages);

	while(i < pages) {
		lt_t loop = page_size * i;
		lt_t start_index = begin+loop;
		lt_t end_index = start_index+page_size;

		if(end_index >= len || end_index >= end)
			break;

		random_touch_page(&addr[start_index], page_size);
		i++;
	}
/*
	while(j) {
		void *p = addr;

		i = rand_intr(begin, end-10);
		//i = i - begin;

		if(i >= len)
			continue;

		if(((char*)p)[i] != 0xf) {
			((char*)p)[i] = 0xf;
			break;
		}
		else {
			((char*)p)[i] = 0x00;
		}

		begin = rand_intr(begin, end-10);

		if((begin >= len) || (begin >= end)) {
			//Once in a while imitating seq behavior 
			int loop = MAX_TRY;
			begin = rand_intr(tmp_begin, end-200);
			while(loop) {
				touch_simple(&p[begin]);	
				loop--;
				begin++;
			}
			begin = tmp_begin;
		}
		j--;
	}
*/
}

static void random_touch_n(char *addr, lt_t len)
{
	lt_t which_slice = 0;
	lt_t switcher = len/2;
	lt_t n = 10;
	
	if(in_transition) {
		n = 2;
	}

	while(n) {
		lt_t start = which_slice? switcher : 0;
		lt_t end = which_slice? len : switcher;	
		random_touch(addr, start, end, len);
		which_slice = !which_slice;
		n--;
	}
}

static void pattern_rand(char *addr, lt_t len)
{
	random_touch_n(addr, len);	
	return;
}

static access_pattern pattern[MEM_MAX] = {
	pattern_fix,
	pattern_stride,
	pattern_seq,
	pattern_repeat,
	pattern_rand
};
/*=================================Access pattern Implementation: End================================= */
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
			//printf("found a vaild file: %s\n", node->path);
			break;
		}
		//printf("not finding the val\n");
		node++;
	}

	if(!node->path) {
		//printf("node->path is NULL\n");
		return NULL;
	}
	return node;
}

static file_path* get_file_path(lt_t *len)
{
	file_path *node = NULL;
	int i;

	/* Normal first best fit for the length*/
	for(i = 0; i<max; i++) {
		//printf("Asked length is : %ld : %ld\n", len, file_lst[i].size);
		if(file_lst[i].size >= *len) {
			//printf("Found\n");
			node = find_avail_node(i);
			if(!node) {
				printf("Size: %ld is filled\n", file_lst[i].size);
				file_lst[i].filled = 1;	
				continue;
			}
			else {
				*len = file_lst[i].size;
				break;
			}
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
		printf("reached maximum phase transitions\n");
		exit(1);
		return;
	}
	curr++;
}

static void alloc_track_init()
{
	memset(&alloc_track, 0x00, sizeof(alloc_track));
}

static int add_new_alloc(char *address, long len, mem_type_t type)
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
	return 1;
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

/* 
	Thought is to maintain 3:1 ratio between anon and file
	Slice defines that ratio: if anon_slice is 0 then only possibility
	is file.
*/
static int randomize_alloc_type(int anon_slice)
{
	if(anon_slice)
		return rand_lim(type_max);
	return file;
}

static lt_t randomize_alloc_len()
{
	lt_t max_req = max_limit * PAGE_SIZE;
	int i = 0;

	if(max_req <= file_lst[0].size)
		return file_lst[0].size;
	else {
		int l = 10;
		while(l) {
			l--;
			i = rand_intr(0, speed);
			if(file_lst[i].size > max_req)
				continue;
			else
				break;
		}
	}
	// printf("ANON LEN: %ld\n", file_lst[i].size);
	return file_lst[i].size;
}

static lt_t randomize_alloc_count(int precision)
{
	lt_t i = 0;
	if(precision)
		i = rand_intr(max_alloc_per_phase/2, max_alloc_per_phase);
	else
		i = rand_intr(5, MAX_TRANSITION_CNT >> 3);

	return i;
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

/*
 * This is the primary holding state of a phase.
 * Within the holding phase, the system generates pattern.
 */
static void touch(lt_t i)
{
	char *addr;
	lt_t len;

	if(curr >=  0) {
		if(i >= 0 && i < alloc_track[curr].list_count) {
			//printf("random touch\n");
			addr = alloc_track[curr].lst[i].addr;
			len = alloc_track[curr].lst[i].len;
			/* TODO: Here is the point where the pattern is defined or 
			 * pattern changes.
			 */
			//printf("addr: %p len: %ld\n", addr, len);
			pattern[access_type](addr, len);
		}
	}
}


/* 
 * Simple loop not touching any pages of the memory.
 * Basically a relax for memory.
 */
static int num[NUMS];
static int loop_once(double end)
{
	int i, j = 0;
	if(end <= 0)
		return j;

	for (i = 0; i < NUMS; i++)
		j += num[i]++;
	return j;
}

static void loop_n(int n)
{
	while(n) {
		lt_t cnt = alloc_track[curr].list_count;
		/* touch only last element*/
		//printf("Index touch is: %ld\n", cnt-1);
		touch(cnt-1);
		//loop_once();
		n--;
	}
}

/*Allocator*/
/*
 * This randomizes the sequence
 * of allocation. 
 *
 */
#define PAGE (sysconf(_SC_PAGESIZE))
#define BYTE_TO_PAGE(B) ((B) / (sysconf(_SC_PAGESIZE)))

char* mmapper(char *path, lt_t size, mem_type_t type)
{
	int fd;
	char *map = NULL;
	//lt_t page_cnt = BYTE_TO_PAGE(size);
	//off_t pa_offset = 0 & ~(sysconf(_SC_PAGE_SIZE) - 1);
	if(anon == type) {
		//printf("ANON SIZE: %ld \n", size);
		map = (char *)mmap(NULL, size,
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		//anon_cnt += page_cnt;
	} else if (file == type) {
		//printf("FILE SIZE: %ld \n", size);
		if(!path) {
			fprintf(stderr, "path is null\n");
			exit(1);
		}
		//printf("PATH: %s\n", path);
		fd = open(path, O_RDWR);
		if(-1 == fd) {
			fprintf(stderr, "path: %s: ", path);
			perror("open is the error");
			exit(1);
		}
		if(ftruncate(fd, size)) {
			fprintf(stderr, "truncate is the error\n");
		}

		map = (char *)mmap(NULL, size,
			 PROT_READ | PROT_WRITE,
			 MAP_PRIVATE, fd, 0);
		//file_cnt += page_cnt;
	}
	if(MAP_FAILED == map) {
		printf("%s, %ld, type: %d\n",path, size, type);
		perror("mmap");
		exit(1);
	}

	if (madvise(map, size, MADV_RANDOM)) {
		perror("madvise");
		exit(1);
	}

	//printf(">>>>ADDR0 : %p\n", map);
	return map;
}

char* alloc(mem_type_t type, lt_t *len, file_path **node)
{
	if(anon == type) {
		*node = NULL;
		if(!(*len)) {
			fprintf(stderr, "Error in length comp:%ld\n", file_lst[0].size);
			*len = file_lst[0].size;				
		}
	}
	else {
		*node = get_file_path(len);
		if(!*node) {
			fprintf(stderr,"path is NULL");
			return NULL;
		}
	}
	if(max_alloc_per_phase_byte && (*len > max_alloc_per_phase_byte))
		*len = max_alloc_per_phase_byte;

	return mmapper(( (*node) ? (*node)->path : NULL),*len, type); 
}

static mem_type_t random_allocator_one( int anon_slice, lt_t *cnt)
{
	int alloc_type = randomize_alloc_type(anon_slice);
	lt_t alloc_len = randomize_alloc_len();
	char *addr;
	file_path *node = NULL;
	int alloc_suc;
	lt_t page_cnt = 0;
	
	addr = alloc(alloc_type, &alloc_len, &node);
	//printf("ADDR1: %p\n", addr);
	if(!addr)
		return type_max;
	//lt_sleep(1);
	alloc_suc = add_new_alloc(addr, alloc_len, alloc_type);

	/* Page count computation */
	page_cnt = BYTE_TO_PAGE(alloc_len);	
	total_alloc_pages += page_cnt;
	if(file == alloc_type)
		file_cnt += page_cnt;
	else if(anon == alloc_type)
		anon_cnt += page_cnt;

	if(node && (alloc_suc > 0)) {
		node->alloc = alloc_suc;
	}
	else if(alloc_suc < 0) {
		fprintf(stderr,"Failed to add new tracker");
		exit(1);
	}

	if(alloc_precision) { 
		if(page_cnt > *cnt)
			*cnt = 0;
		else 
			*cnt -= (page_cnt);
	}
	else {
		//printf("cnt : %ld\n", *cnt);
		*cnt = *cnt - 1;
	}
	//curr_alloc += (alloc_len / PAGE_SIZE);
	return alloc_type;
}

/* 
 * Core tansition function
 */
static int trans_rand_alloc()
{
	#define MAX_LOOP 3
	lt_t cnt = 0;
	int i = rand_lim(MAX_LOOP);
	int anon_slice;
	int ret = 0;
	printf("+++++++++++++++++++++++TRANSITION: Start+++++++++++++++++++++++++\n");	
	go_to_nxt_phase();

	if(alloc_precision)
		cnt = randomize_alloc_count(1);
	else
		cnt = randomize_alloc_count(0);

	anon_slice = cnt/4;
	//printf("anon slice %d\n", anon_slice);
	in_transition = 1; /* FIXME: This is bad, needs a better approach to handle tranition restirtion*/
	while(cnt /*&& (curr_alloc < max_limit)*/) {
		mem_type_t typ;
		typ = random_allocator_one(anon_slice, &cnt);
		if(typ >= type_max) { /*error check, sometimes path will get exhausted */
			cnt--;
			ret++;
			continue;
		}
		loop_n(i); /* Small loop that touches only the last to give reality*/
		if(typ > file) {
			anon_slice--;
		}
		//cnt--;
	}
	in_transition = 0;

	printf("+++++++++++++++++++++++TRANSITION: end+++++++++++++++++++++++++\n");	
	if(ret) {
		ret = -1;
	}
	return (ret);
}

static int loop_for(double exec_time)
{
	double last_loop = 0, loop_start;
	int tmp = 0;
	double start = 0;
	double now = cputime();
	double run = 0;
	lt_t i = alloc_track[curr].list_count;
	
	printf("list count: %ld\n", i);
	while ( exec_time > (start + run) ) {
		loop_start = now;
		//printf(" looping\n");		
		/* touch the index i @ curr phase */
		if(i > 0) {
			//printf("touch\n");
			touch(i-1);
			i--;
		}
		else {
			i = alloc_track[curr].list_count;
		}
		tmp += loop_once(exec_time - run);

		now = cputime();
		last_loop = now - loop_start;
		run += last_loop;
		/*if (emergency_exit && wctime() > emergency_exit) {
			fprintf(stderr, "oops!!!: memspin/%d emergency exit!\n", getpid());
			fprintf(stderr, "Something is seriously wrong! Do not ignore this.\n");
			break;
		} */
	}
	return tmp;
}
/* Each job is a phase: transition and holding */
static int job(double exec_time)
{
	double chunk1, chunk2;
	lt_t i;
	double program_end = wctime() + exec_time;

	if (wctime() > program_end)
		return 0;
	else {
		/* TRANSITION: Randomize and touch allocations */
		if(!alloc_nomore) {
			if(trans_rand_alloc())
				goto FAIL;
		}

		i = alloc_track[curr].list_count;
		printf("ENTERING A PHASE:  %f\n", cputime());
		printf(" PHASE list count %ld\n", i);
		//chunk1 = drand48() % exec_time;
		//chunk2 = exec_time - chunk1;

		//printf("%f %f\n", chunk1, chunk2);
		/* HOLDING: Loop and touch imm allocated pages */
		loop_for(exec_time);
/*
		if(wctime() > program_end) {
			printf("Exit before the job loop2\n");
			return 0;
		}
		loop_for(chunk2, program_end );
		if(wctime() > program_end) {
			printf("Exit before the job loop3\n");
			return 0;
		}
		loop_for(chunk2, program_end );
		if(wctime() >= program_end)
			printf(" Overrun by: %f\n", wctime() - program_end);
*/
		printf("EXITING A PHASE: %f\n", cputime());
		phase_cnt++;
		return 1;
	}
FAIL:
	return 0;
}

__attribute__((destructor)) void on_process_exit()
{
	printf("Calling After exit call\n");
	getrusage(RUSAGE_SELF, &res);
	printf("%d, %ld, %ld, %ld, %ld\n", getpid(), file_cnt, anon_cnt, res.ru_minflt, res.ru_majflt);	
}

#define OPT "vp:l:e:M:s:t:A:V:h"

static void usage()
{
	printf("memgobble\n");
	printf("memgobble [options]\n");
	printf("List of options:\n");
	printf("\t-h                   : Display usage\n");
	printf("\t-v                   : Switch verbose off\n");
	printf("\t-p <locality count>  : Maximum locality the system will run\n");
	printf("\t-l <upper limit>     : Set the upper bound of alloc request\n");
	printf("\t-e <Execution time>  : Execution time of each locality in Ms\n");
	printf("\t-M <pages>           : Maximum allocation in pages per phase\n");
	printf("\t-s <speed>           : Speed at which memory is gobbled\n");
	printf("\t\t speed: 0-n, Higher the value faster the memory exhaustion\n");
	printf("\t-t <access-type>     : Access type of the locality\n");
	printf("\t\t access-type:\n");
	printf("\t\t\t0: Fix memory access pattern\n");
	printf("\t\t\t1: Stride memory access pattern\n");
	printf("\t\t\t2: Sequential memory access pattern\n");
	printf("\t\t\t3: Repeat memory access pattern\n");
	printf("\t\t\t4: Random memory access pattern\n");
	printf("\t-V <File name>       : Vector of localities\n");
}

int main(int argc, char** argv)
{
	double wcet_ms, period_ms;
	double wcet = 0;
	double duration = 0, start = 0;
	double scale = 1.0;
	int cur_job = 0, num_jobs = 0;
	int verbose = 1;
	lt_t max_phase = MAX_ALLOC; /* maximum number of phases allowed*/ 
	double cs_length = 1; /* millisecond */
	int c;

	if(argc > 1) {
		while((c = getopt(argc, argv, OPT)) != -1) {
			switch(c) {
			case 'p': /* Maximum number of locality system will */
				max_phase = atol(optarg);
			break;
			case 'l':
				max_limit = atol(optarg);
			break;
			case 'v':
				verbose = 0;
			break;
			case 'e':
				/* Execution  Time*/
				wcet = atol(optarg);
			break;
			case 'M': /* Maximum Alloc per phase */
				max_alloc_per_phase = atol(optarg);
				alloc_precision = true;
				max_alloc_per_phase_byte = max_alloc_per_phase * PAGE_SIZE;
				printf("max alloc per phase: %ld:\n", max_alloc_per_phase_byte);
				//if(max_alloc_per_phase > MAX_TRANSITION_CNT)
				//	max_alloc_per_phase = MAX_TRANSITION_CNT;
			break;
			case 's':
				/* higher the speed filling is faster */
				speed = atol(optarg);
				if(speed >= max)
					speed = max-1;
			break;
			case 't':
				access_type = atol(optarg);
				if(access_type >= MEM_MAX) {
					fprintf(stderr, "Wrong access type\n");
					exit(1);
				}
			break;
			case 'A': /* Total alloc in all phase put together*/
				alloc_size = atol(optarg);
				alloc_precision = true;	
				printf("Warning: Allocate request is now precise: %ld\n", alloc_size);
			break;
			case 'V': /* Vector of localities */
				vector = 1;
				if(vector_init(optarg)) {
					printf("Please provide valid file\n");
				}
			break;

			case 'h':
			default:
				usage();
				exit(1);
			break;

			}
		}
	}

	progname = argv[0];
	if(wcet)
		wcet_ms = wcet;
	else
		wcet_ms = 55;

	/* Harcoded value */
	period_ms = 50 * wcet_ms;
	duration = wcet_ms;

	start = wctime();

	alloc_track_init();

	printf("\nFORMAT:\n");
	printf("Metric: Pages of size 4k\n");
	printf("<PID>, <PHASE CNT>,  <DURATION>,  <FILEMAPCNT>, <ANONMAPCNT>, <TOTAL CNT>, <MINFAULT>, <MAJFAULT>, <TOTALFAULT>, <RSS(pages)>\n");
	do {
		struct rusage res1;

		if(vector) {
				
		}
		getrusage(RUSAGE_SELF, &res1);
		if (verbose) {

			printf("%d, %d, %.4fms, %ld, %ld, %ld, %ld, %ld, %ld, %ld\n", 
				getpid(),
				cur_job,
				(wctime() - start) * 1000,
				file_cnt, anon_cnt,
				file_cnt + anon_cnt,
				res1.ru_minflt, res1.ru_majflt,
				res1.ru_minflt + res1.ru_majflt,
				(res1.ru_maxrss/4));
		}

		if(max_limit < (file_cnt + anon_cnt)) {
			alloc_nomore = 1;
			printf("reached more than the max_limit(%ld): %ld", max_limit, total_alloc_pages);
		}

		cur_job++;
		if( (max_phase <= cur_job) /*|| (alloc_nomore)*/ )
			break;

		/* convert to seconds and scale */
	} while (job(wcet_ms));

	return 0;
}

