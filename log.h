#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#define PR_DEBUG(fmt, ...)					\
       do {							\
		fprintf(stdout,fmt, ##__VA_ARGS__);		\
	} while(0)
#else

#define PR_DEBUG(fmt, ...) 

#endif

#define PR_ERROR(fmt, ...)					\
       do {							\
		fprintf(stderr,"Error:"fmt, ##__VA_ARGS__);	\
	        exit(-1);					\
	} while(0)

#define PR_WARN(fmt, ...)					\
       do {							\
		fprintf(stderr, "Warning:"fmt, ##__VA_ARGS__);	\
	} while(0)


typedef unsigned long lt_t;
typedef unsigned long long u64;

#endif
