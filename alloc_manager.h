#ifndef __ALLOC_MANAGER_H_
#define __ALLOC_MANAGER_H_

typedef struct alloc_list_per_se {
	char *addr;	/* starting address */
	lt_t len;	/* length of the mmap*/
	mem_type_t type; /* type of allocation*/
}alloc_list_per_se_t;

/* Per phase allocation list */
typedef struct alloc_list {
	lt_t list_count;
#define MAX_ALLOC_PER_SE (500)
	alloc_list_per_se_t lst[MAX_ALLOC_PER_SE];
}alloc_list_t;


typedef enum mem_type {
	file = 0,
	anon,
	type_max
}mem_type_t;

#define mem_type_string(x)	( ((x) == file)? "File" : "Anon")

void alloc_mgr_init();

int alloc_mgr_trans_alloc();

void alloc_mgr_touch_curr();

#endif
