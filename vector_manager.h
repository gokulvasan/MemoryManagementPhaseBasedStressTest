#ifndef __VECTOR_MANAGER_H__
#define __VECTOR_MANAGER_H__
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <unistd.h>
#include "log.h"

vector_mgr_init(char *path);

size_t vector_mgr_get_nxt( lt_t *alloc_cnt, double *wcet, lt_t is_ipfvt);

#endif
