
#include "time_manager.h"
#include "log.h"
#include "alloc_manager.h"



static int transition() {

	trans_rand_alloc();
	return 0;
}


int stabilise(double exec_time, time_granularity t)
{
	double start = cputime(t);
	double job_end = start + exec_time;

	while(cputime(t) < job_end) {
		touch_once_curr();
	}
}

int job(double exec_time, time_granularity t)
{
	int ret = 0;

	ret = transition();
	if(!ret) {
		ret = stablilise(exec_time, time_granularity);
	}

	return ret;
}
