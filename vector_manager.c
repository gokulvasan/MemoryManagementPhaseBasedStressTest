#include "vector_manager.h"

static FILE *vector_stream = NULL;

static void _reset_stma_ltma(u64 stma, u64 ltma)
{

#define PROC "/proc/"
#define PD_LEN "/pd_len"
#define PATH_LEN 256

	FILE *fd;
	pid_t pid = getpid();
	char path[PATH_LEN] = {'\0'};
	unsigned long long data[2] = {0x00};

	sprintf(path, "%s%d%s", PROC, pid, PD_LEN);

	fd = fopen(path, "rw");
	if(!fd) {
		PR_ERROR("Something is Wrong in opening the file\n");
	}

	fread(data, sizeof(data), 1, fd);	
	fclose(fd);

	PR_DEBUG("The old Value is: STMA: %llu LTMA: %llu\n", data[0], data[1]);

	data[0] = stma;
	data[1] = ltma;
	fd = fopen(path, "wb");
	if(!fd) {
		PR_ERROR("Something is Wrong in opening the file\n");
	}
	if(!fwrite(data, sizeof(data), 1, fd)) {
		perror("Write is not working :(");
		exit(-1);
	}
	fclose(fd);

	fd = fopen(path, "rb");
	memset(data, 0x00, sizeof(data));
	fread(data, sizeof(data), 1, fd);

	if((data[0] - stma) || (data[1] - ltma)) {	
		PR_ERROR("Value not set right STMA:%llu(E%llu) LTMA:%llu(E%llu)\n",
			data[0], stma, data[1], ltma);
	}
	fclose(fd);
}
static void reset_stma_ltma(u64 stma, u64 ltma)
{
	u64 stma_f = stma;
	u64 ltma_f = ltma;
	
	if(!stma_f && ltma_f) {
		stma_f = ltma_f/2;
	}
	else if(stma_f && !ltma_f) {
		ltma_f = stma_f * 2;
	}
	else if(!stma_f && !ltma_f) {
		PR_ERROR("Expect Non Zero Values\n");
	}
	
	if(stma_f && ltma_f) {
		_reset_stma_ltma(stma_f, ltma_f);
	}
	else {
		PR_ERROR("Something wrong\n");
	}
}


/*
	For now we will vectorize only wcet & count.
	getline
	strtok
	Format of csv: PHASE == <allocation_count,wcet>
		       IPFVT == <index_count"\t"IPFVT>
*/
size_t vector_mgr_get_nxt( lt_t *alloc_cnt, double *wcet, lt_t is_ipfvt )
{

#define PHASE_DELIMITER ","
#define IPFVT_DELIMITER "\t"

	char *line = NULL;
        size_t len = 0;
	ssize_t nread = 0;
	char *t;

	if((nread = getline(&line, &len, vector_stream) != -1)) {

		t = strtok(line, is_ipfvt ?  IPFVT_DELIMITER : PHASE_DELIMITER);
		if(is_ipfvt) {
			*alloc_cnt = 1;
		} else {
			*alloc_cnt = atol(t);
		}
		t = strtok(NULL, is_ipfvt ?  IPFVT_DELIMITER : PHASE_DELIMITER);
		*wcet = atof(t);

		PR_DEBUG("Vector_mgr: allocation_count:%ld Wcet: %fms\n",
							 *alloc_cnt, *wcet);
		free(line);
	}
	return nread;
}

void vector_mgr_init(char *path)
{
	if(!path) {
		PR_ERROR("%s:Path is NULL\n", __func__);
	}

	vector_stream = fopen(path, "r");
	if(!vector_stream)
		PR_ERROR("%s: %s Failed to open\n", __func__, path);
}

