#ifndef __IO_H
#define __IO_H

#include "ipc.h"

#define MAX_N 10
unsigned int processes_count;
local_id current;

FILE * fp;
int input[MAX_N][MAX_N];
int output[MAX_N][MAX_N];

#endif
