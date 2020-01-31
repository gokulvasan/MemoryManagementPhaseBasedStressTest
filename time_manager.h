#ifndef __ALLOC_TIME_H__
#define __ALLOC_TIME_H__

typedef unsigned long lt_t;
typedef unsigned long long u64;

typedef enum {
	T_SECS = 0, 	/* Time Granularity in Seconds */
	T_milliSECS,    /* Time Granularity in milliSeconds */	
	T_microSECS,	/* Time Granularity in microSeconds */
	T_nanoSECS,	/* Time Granularity in nanoSeconds */
	T_END
}time_granularity;

#define TIME_TO_STRING(x) ( (T_SECS == (x))? "s" : 		\
			((T_milliSECS == (x)) ? "ms" : 		\
		 	((T_microSECS == (x)) ? "us" : "ns")))

typedef struct _conv {
	lt_t secs;
	lt_t microsecs;
	lt_t nanosecs;
}conv;

static conv converter[T_END] = {
			{1, 1E-6, 1E-9},
			{1E3, 1E-3, 1E-6},
			{1E6, 1, 1E-3},
			{1E9, 1E3, 1}
			};


int lt_sleep(lt_t timeout);

double wctime(time_granularity t);

/* CPU time consumed so far in milliseconds */
double cputime(time_granularity t);

#endif
