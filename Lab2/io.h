#ifndef __IO_H
#define __IO_H

#include "ipc.h"
#include "banking.h"

#define MAX_N 10
unsigned int processes_count;

FILE* fp;
int input[MAX_N][MAX_N];
int output[MAX_N][MAX_N];

balance_t banks[10];
typedef struct{ BalanceHistory balanceHistory; local_id current; AllHistory allHistory;} bank;

#endif
