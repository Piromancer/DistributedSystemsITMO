#ifndef __IO_H
#define __IO_H

#include <fcntl.h>
#include <getopt.h>
#include "ipc.h"
#include "banking.h"

#define MAX_N 10
unsigned int processes_count;

FILE* fp;
int input[MAX_N][MAX_N];
int output[MAX_N][MAX_N];

balance_t banks[10];
typedef struct{ BalanceHistory balanceHistory; local_id current; AllHistory allHistory; timestamp_t lamp_time;} bank;
bank target;
pid_t pids[MAX_N];

int mutexl_flag = 0;
static struct option long_options[] = {
        {"proc", required_argument, 0, 'p'},
        {"mutexl", no_argument, &mutexl_flag, 1},
        {NULL, 0, 0, 0}
    };

#endif