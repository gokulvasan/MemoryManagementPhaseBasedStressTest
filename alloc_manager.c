#include "alloc_manager.h"
#include "log.h"

/* Allocation tracker */
/* List of phases */
signed long curr = -1; 		  /*current phase index*/
static alloc_list_t *alloc_track; /*Alloc Tracker*/

/*
 * Mallocing because data segment is exceeding 2G error.
 */
#define ALLOC_INIT() do {							\
		alloc_track = malloc(sizeof(*alloc_track) * MAX_ALLOC);		\
		if(!alloc_track) {						\
			fprintf(stderr, "Malloc Failure for alloc_track\n");	\
			exit(-1);						\
		}								\
	} while(0)

static lt_t max_limit = 1; /* Maximum allocation in page cnt */
#define PAGE_SIZE 4096

/*
 * Statistics: 
 *	counting Anon and Filemapped pages
 */
static lt_t anon_cnt = 0;
static lt_t file_cnt = 0;
static lt_t total_alloc_pages=0;  /* Tells, pages the gobbler really allocated */

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
		PR_ERROR("reached maximum phase transitions\n");
	}
	curr++;
}

void alloc_track_init()
{
	if(!alloc_track) {
		ALLOC_INIT();
	}
	memset(alloc_track, 0x00, sizeof(*alloc_track) * MAX_ALLOC);
}

static int add_new_alloc(char *address, long len, mem_type_t type)
{
	lt_t i = alloc_track[curr].list_count;

	if(i >= MAX_ALLOC_PER_SE) {
		PR_WARN("Curr: %ld MAX alloc : %ld\n", curr, i);
		return -1;
	}
	alloc_track[curr].lst[i].addr = address;
	alloc_track[curr].lst[i].len = len;
	alloc_track[curr].lst[i].type = type;
	alloc_track[curr].list_count++;
	PR_DEBUG("New Alloc at addr:%p Len:%ld Type:%s\n", address, len, mem_type_string(type));
	return 1;
}

/* 
	Thought is to maintain 3:1 ratio between anon and file
	Slice defines that ratio: if anon_slice is 0 then only possibility
	is file.
*/
#define FILE_RATIO 4

static mem_type_t randomize_alloc_type()
{
	static int cnt = 0;
	mem_type_t type = type_max;

	if ((!(cnt%FILE_RATIO)) && cnt) {
		type = anon;
	} else {
		type = file;
	}
	cnt++;

	return type;
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

	return file_lst[i].size;
}

static lt_t randomize_alloc_count(int precision)
{
	lt_t i = 0;

	if(precision)
		i = max_alloc_per_phase;
	else
		i = rand_intr(5, MAX_TRANSITION_CNT >> 3);

	return i;
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

	if(anon == type) {
		map = (char *)mmap(NULL, size,
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	} else if (file == type) {
		if(!path) {
			PR_ERROR("path is null\n");
		}
		fd = open(path, O_RDWR);
		if(-1 == fd) {
			perror("open is the error");
			PR_ERROR("Failed path: %s: ", path);
		}
		if(ftruncate(fd, size)) {
			PR_WARN("truncate is the error\n");
		}

		map = (char *)mmap(NULL, size,
			 PROT_READ | PROT_WRITE,
			 MAP_PRIVATE, fd, 0);
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

	return map;
}

char* alloc(mem_type_t type, lt_t *len, file_path **node)
{
	if(anon == type) {
		*node = NULL;
		if(!(*len)) {
			PR_WARN("Error in length comp:%ld\n", file_lst[0].size);
			*len = file_lst[0].size;				
		}
	}
	else {
		*node = get_file_path(len);
		if(!*node) {
			PR_WARN("path is NULL");
			return NULL;
		}
	}
	if(max_alloc_per_phase_byte && (*len > max_alloc_per_phase_byte))
		*len = max_alloc_per_phase_byte;

	return mmapper(( (*node) ? (*node)->path : NULL),*len, type); 
}

static mem_type_t random_allocator_one ()
{
	mem_type_t alloc_type = randomize_alloc_type();
	lt_t alloc_len = randomize_alloc_len();
	char *addr;
	file_path *node = NULL;
	int alloc_suc;
	lt_t page_cnt = 0;

	addr = alloc(alloc_type, &alloc_len, &node);
	if(!addr)
		return type_max;

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
		PR_ERROR("Failed to add new tracker");
	}

	return alloc_type;
}

/* 
 * Core tansition function
 */
int trans_rand_alloc()
{
	int ret = 0;
	mem_type_t typ;

	go_to_nxt_phase();
	typ = random_allocator_one();
	if(type_max == typ) {
		PR_ERROR("Failure in Transition allocation\n");
	}
	return ret;
}

void touch_once_curr()
{
	char *addr = alloc_track[curr].lst[0].addr;
	lt_t len = alloc_track[curr].lst[0].len;

	addr = addr + (len/2);      /* Touch somewhere between */
	pattern_mgr_fix(addr, len/2);	
}

