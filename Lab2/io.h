#ifndef __IO_H
#define __IO_H

#include <fcntl.h>
#include "ipc.h"
#include "banking.h"

#define MAX_N 10
unsigned int processes_count;

FILE* fp;
int input[MAX_N][MAX_N];
int output[MAX_N][MAX_N];

balance_t banks[10];
typedef struct{ BalanceHistory balanceHistory; local_id current; AllHistory allHistory;} bank;
bank target;
pid_t pids[MAX_N];

void parent_start(bank* cur);
void child_start (bank* cur, balance_t init_bal);

#endif
