#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>

#include "io.h"
#include "ipc.h"
#include "pa2345.h"
#include "banking.h"

void close_unused_pipes(){
    for (unsigned int src = 0; src < processes_count; src++) {
        for (unsigned int dst = 0; dst < processes_count; dst++) {
            if (src != current && dst != current && src != dst) {
                close(output[src][dst]);
                close(input[src][dst]);
            }
            if (src == current && dst != current) {
                close(input[src][dst]);
            }
            if (dst == current && src != current) {
                close(output[src][dst]);
            }
        }
    }
}


void display_usage(){
    puts( "-p X Y..Y, where X - number of processes 0..10, Y - start balance for each process" );
}


//send_msg
void send_msg(local_id c_id, int16_t type, const char* const log) {
    if (c_id != PARENT_ID){
        Message msg = { .s_header = { .s_magic = MESSAGE_MAGIC, .s_type = type, }, };
        printf(log, current, getpid(), getppid());
        fprintf(fp, log, current, getpid(), getppid());
        sprintf(msg.s_payload, log, c_id, getpid(), getppid());
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        send_multicast(NULL, &msg);
    }
}

//receive_msg
void receive_msg(local_id c_id, unsigned int child_processes_count){
    for (int i = 1; i <= child_processes_count; i++){
        Message msg;
        if (i != c_id){
            receive(NULL, i, &msg);
        }
    }
}


/* from start file pa2345.h
#include "banking.h"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:D:\\CLionProjects\\DSI";
###
LD_PRELOAD=D:/CLionProjects/DSI/Lab2/lib64 ./pa2 -p 2 10 20



int main(int argc, char * argv[])
{
    //bank_robbery(parent_data);
    //print_history(all);

    return 0;
}*/

void transfer(void* parent_data, local_id src, local_id dst, balance_t amount)
{
    local_id* cur = parent_data;
    Message msg;
    {
        msg.s_header = (MessageHeader) { .s_local_time = get_physical_time(), .s_magic = MESSAGE_MAGIC, .s_type = TRANSFER, .s_payload_len = sizeof(TransferOrder)};
        TransferOrder order = { .s_src = src, .s_dst = dst, .s_amount = amount };
        memcpy(&msg.s_payload, &order, sizeof(TransferOrder));
        send(cur, src, &msg);
    }
    //receive proof
    receive(cur, dst, &msg);
}

void hist(local_id cur, balance_t balance){
    ;
}

int main( int argc, char* argv[] ){
    unsigned int children_processes_count;
    fp = fopen("events.log", "w");
    int opt = 0;
    static const char* optString = "p:?";
    opt = getopt( argc, argv, optString );
    while( opt != -1 ) {
        switch( opt ) {
            case 'p':
                children_processes_count = strtoul(optarg, NULL, 10);
                for (int i = 0; i < children_processes_count; i++){
                    banks[i] = strtoul(argv[i+3], NULL,10);
                }
                break;
            case '?':
                display_usage();
                return 1;
            default:
                puts( "Error - unknown error" );
                return 1;
        }
        opt = getopt( argc, argv, optString );
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
                current = id;
                break;
            } else {
                current = PARENT_ID;
            }
        } else {
            puts("Can't create process!");
        }

    }
    close_unused_pipes();

    send_msg(current, STARTED, log_started_fmt);
    receive_msg(current, children_processes_count);
    printf(log_received_all_started_fmt, current);
    fprintf(fp, log_received_all_started_fmt, current);
    send_msg(current, DONE, log_done_fmt);
    printf(log_received_all_done_fmt, current);

    if (current == PARENT_ID) {
        for (int i = 1; i <= processes_count; i++) {
            waitpid(proc_pids[i], NULL, 0);
        }
    }
    fclose(fp);
    return 0;
}
