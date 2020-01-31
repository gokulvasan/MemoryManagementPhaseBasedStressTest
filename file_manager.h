#ifndef __FILE_MANAGER_H__
#define __FILE_MNAGER_H__

#include <sys/resource.h> // needed for getrusage 
#include <unistd.h>

#define FILE_MAX 500000

/* This defines maximum allocation in metric of pages in a transition
 *
 */
#define MAX_TRANSITION_CNT 150
#define MAX_TRANS_HALF (MAX_TRANSITION_CNT/2)

/*
 * Basically tries to maintain
 * array of list of paths and its
 * access. * The array itself is fed by files.h
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

void file_mgr_set_file_cnt(rlim_t file_max);

file_path* file_mgr_get_file(lt_t *len)

#endif
