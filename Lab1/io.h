#ifndef __IO_H
#define __IO_H

#include "ipc.h"

#define MAX_N 10
size_t processes_count;
local_id current;

int output[MAX_N][MAX_N];
int input[MAX_N][MAX_N];

#endif