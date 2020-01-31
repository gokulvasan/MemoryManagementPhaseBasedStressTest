#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <sys/time.h>
#include <sys/types.h>

#define ms2ns(ms) ((ms)*1000000LL)

int lt_sleep(lt_t timeout)
{
	struct timespec delay;

	delay.tv_sec  = timeout / 1000000000L;
	delay.tv_nsec = timeout % 1000000000L;
	return nanosleep(&delay, NULL);
}

/* wall-clock time in xxxseconds, where xxx == ms | micro | none*/
double wctime(time_granularity t)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * converter[t].secs) + 
		(tv.tv_usec * converter[t].microsecs));
}

/* CPU time consumed so far in milliseconds */
double cputime(time_granularity t)
{
	struct timespec ts;
	int err;
	err = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	if (err != 0)
		perror("clock_gettime");

	return ((ts.tv_sec * converter[t].secs) + 
		(ts.tv_nsec * converter[t].nanosecs));
}

#if 1 //testing: START

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

#endif //testing: END


