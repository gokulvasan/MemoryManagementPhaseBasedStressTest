#include "file_manager.h"

/*
 * Concatenator 
 */
#define FS "/media/test_images/"
#define PATH(name) FS # name
#include "files.h"

void file_mgr_set_file_cnt(rlim_t file_max)
{
	struct rlimit rlim;

	rlim.rlim_cur = file_max;
	rlim.rlim_max = file_max;
	if(!setrlimit(RLIMIT_NOFILE, &rlim)) {
		perror("setting rlimit\n");
	}
}

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

	if(!node->path) {
		PR_WARN("node->path is NULL\n");
		return NULL;
	}
	return node;
}

file_path* file_mgr_get_file(lt_t *len)
{
	file_path *node = NULL;
	int i;

	/* Normal first best fit for the length*/
	for(i = 0; i<max; i++) {
		if(file_lst[i].size >= *len) {
			node = find_avail_node(i);
			if(!node) {
				PR_WARN("Size: %ld is filled\n", file_lst[i].size);
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

