#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>

#include "io.h"
#include "ipc.h"
#include "pa2345.h"





void close_unused_pipes() {
    bank* cur = &target;
    for (unsigned int src = 0; src < processes_count; src++) {
        for (unsigned int dst = 0; dst < processes_count; dst++) {
            if (src != cur->current && dst != cur->current && src != dst) {
                close(output[src][dst]);
                close(input[src][dst]);
            }
            if (src == cur->current && dst != cur->current) {
                close(input[src][dst]);
            }
            if (dst == cur->current && src != cur->current) {
                close(output[src][dst]);
            }
        }
    }
}

timestamp_t get_lamport_time(){
    return target.lamp_time;
}


void display_usage(){
    puts( "-p X, where X - number of processes 0..10" );
}


int main( int argc, char* argv[] ){
    mutexl_flag = 0;
    bank* cur_bank = &target;
    unsigned int children_processes_count;
    //int mutexl = 1;
    fp = fopen("events.log", "w");
    int opt = 0;
    static const char* optString = "p:?:";
    int option_index = 0;
    static struct option long_option[] = {
            {"proc", required_argument, 0, 'p'},
            {"mutexl", no_argument, &mutexl_flag, 1},
            {NULL, 0, 0, 0}
    };
    opt = getopt_long(argc, argv, optString, long_option, &option_index);
    while(opt != -1) {
        switch(opt) {
            case 0:
                puts("flag mutexl set");
                printf("%d", mutexl_flag);
                break;
            case 'p':
                children_processes_count = strtoul(optarg, NULL, 10);
                break;
            case '?':
                display_usage();
                return 1;
            default:
                puts("mda");
                return 1;
        }
        opt = getopt_long( argc, argv, optString, long_option, &option_index );
    }
    processes_count = children_processes_count + 1;

    pid_t proc_pids[MAX_N+1];

    //pipes
    for (int src = 0; src < processes_count; src++) {
        for (int dst = 0; dst < processes_count; dst++) {
            if (src != dst){
                int fld[2];
                pipe(fld);
                input[src][dst] = fld[0];
                output[src][dst] = fld[1];
                unsigned int flags1 = fcntl(fld[0], F_GETFL, 0);
                unsigned int flags2 = fcntl(fld[1], F_GETFL, 0);
                fcntl(fld[0], F_SETFL, flags1 | O_NONBLOCK);
                fcntl(fld[1], F_SETFL, flags2 | O_NONBLOCK);
                //log?
            }
        }
    }


    proc_pids[PARENT_ID] = getpid();
    // create porcessess
    for (int id = 1; id <= children_processes_count; id++) {
        int child_pid = fork();
        if (child_pid != -1) {
            if (child_pid == 0) {
                cur_bank->current = id;
                break;
            } else {
                cur_bank->current = PARENT_ID;
            }
        } else {
            puts("Can't create process!\n");
        }

    }
    close_unused_pipes();

    /*send_msg(current, STARTED, log_started_fmt);
    receive_msg(current, children_processes_count);
    printf(log_received_all_started_fmt, current);
    */

    //output loop
    char str[128];
    int num_prints = cur_bank->current * 5;

    for (int i = 1; i <= num_prints; ++i) {
        memset(str, 0, sizeof(str));
        sprintf(str, log_loop_operation_fmt, cur_bank->current, i, num_prints);
        print(str);
    }


    //pthread_mutex_lock (&fp);
    //fprintf(fp, log_received_all_started_fmt, cur_bank->current);
    //pthread_mutex_unlock (&fp);
    //send_msg(current, DONE, log_done_fmt);
    //printf(log_received_all_done_fmt, cur_bank->current);

    if (cur_bank->current == PARENT_ID) {
        for (int i = 1; i <= processes_count; i++) {
            waitpid(proc_pids[i], NULL, 0);
        }
    }
    fclose(fp);
    return 0;
}
