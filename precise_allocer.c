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

#include "alloc_time.h"
#include "log.h"
#include "file_manager.h"

#define NUMS 4096
#define MAX_ALLOC (60000)

/* MAX number of pages*/ 
#define MAX_LIMIT 100000000
#define PAGE_SIZE 4096

/*Enable this for ubuntu or other machines for verbose*/
#if 1

#ifdef DEBUG
#undef DEBUG
#endif

#else

#define DEBUG

#endif

static lt_t vector = 0;		/* Vector of files*/
static lt_t ipfvt  = 0;		/* if(ipfvt){ then the file is series of IPFVT }*/
static lt_t time_g = 0;		/* Decision on Time Granularity */
static u64 stma = 0;		/* STMA reset length		*/
static u64 ltma = 0;		/* LTMA reset length		*/


#define OPT "IvV:hS:L:"
static void usage()
{
	printf("memgobble\n");
	printf("memgobble [options]\n");
	printf("List of options:\n");
	printf("\t-h                   : Display usage\n");
	printf("\t-v                   : Switch verbose off\n");
	rintf("\t-V <File name>	: Vector of localities\n");
	printf("\t-I		: Vector is IPFVT\n");
	printf("\t if CUSTOM_LINUX_KERNEL defined, then\n");
	printf("\t\t -S <STMA Count>	: STMA Value\n");
	printf("\t\t -L <LTMA Count>	: LTMA Value\n");
	printf("\t endif\n");
}

static void Init()
{
	alloc_mgr_init();
	file_mgr_set_file_cnt(FILE_MAX);
	
	if(stma || ltma) {
		reset_stma_ltma(stma, ltma);
	}
}

int main(int argc, char** argv)
{
	double wcet;
	double start = 0;
	int cur_job = 0;
	int verbose = 1;
	int vector = 0;
	int c;

	if(argc > 1) {
		while((c = getopt(argc, argv, OPT)) != -1) {
			switch(c) {
			case 'v':
				verbose = 0;
			break;
			case 'V': /* Vector of localities */
				vector = 1;
				if(locality_vector_init(optarg)) {
					PR_ERROR("Please provide valid file\n");
				}
			break;
			case 'I': /* Vector is IPFVT */
				ipfvt = 1;
				time_g = T_nanoSECS; 
			break;
			case 'S':
				stma = atoll(optarg);		
			break;
			case 'L':
				ltma = atoll(optarg);
			break;
			case 'h':
			default:
				usage();
				exit(1);
			break;
			}
		}
	}
	
	init();

	start = time_mgr_wctime(time_g);
	printf("\nFORMAT:\n");
	printf("Metric: Pages of size 4k\n");
	printf("<PID>, <PHASE CNT>,  <DURATION>,  <FILEMAPCNT>, <ANONMAPCNT>, <TOTAL CNT>, <MINFAULT>, <MAJFAULT>, <TOTALFAULT>, <RSS(pages)>\n");
	do {
		struct rusage res1;

		if(vector) {
			lt_t alloc_cnt = 0;
			if(get_nxt_alloc(&alloc_cnt, &wcet_ms, ipfvt)) {
				reset_max_alloc_per_phase(alloc_cnt);
			}
			else {
				break;
			}
		}

		getrusage(RUSAGE_SELF, &res1);
		if (verbose) {

			printf("%d, %d, %.4f%s, %ld, %ld, %ld, %ld, %ld, %ld, %ld\n", 
				getpid(),
				cur_job,
				(wctime(time_g) - start), TIME_TO_STRING(time_g),
				file_cnt, anon_cnt,
				file_cnt + anon_cnt,
				res1.ru_minflt, res1.ru_majflt,
				res1.ru_minflt + res1.ru_majflt,
				(res1.ru_maxrss/4));
		}
		cur_job++;

	} while (job(wcet_ms, time_g));

	return 0;
}

