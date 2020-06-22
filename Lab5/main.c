#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdbool.h>


#include "io.h"
#include "ipc.h"
#include "pa2345.h"

int deferred[MAX_N];

int request_cs(const void * self){

    bank* cur_bank = (bank*) self;

    timestamp_t reqTime = ++cur_bank->lamp_time;
    Message msg = {
        .s_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_type  = CS_REQUEST,
            .s_local_time = get_lamport_time(),
            .s_payload_len = 0
        },
        .s_payload = "",
    };

    send_multicast(cur_bank, &msg);

    int wait_reply = processes_count - 2;
    int running_processes = processes_count - 1;


    while (running_processes > 0){
        Message msg1;
        if(wait_reply == 0){
            int num_prints = cur_bank->current * 5;
            char str[128];
            for (int i = 1; i <= num_prints; ++i) {
                sprintf(str, log_loop_operation_fmt, cur_bank->current, i, num_prints);
                fprintf(fp, log_loop_operation_fmt, cur_bank->current, i, num_prints);
                print(str);
            }
            running_processes--;
            wait_reply--;
            release_cs(cur_bank);
        }
        int id = receive_any(cur_bank, &msg1);
        if(cur_bank->lamp_time < msg1.s_header.s_local_time)
            cur_bank->lamp_time = msg1.s_header.s_local_time;
        cur_bank->lamp_time++;switch(msg1.s_header.s_type){
            case CS_REPLY:
                wait_reply--;
                break;
            case CS_REQUEST:
                //printf("Received from process %d with time %d, while %d time is %d. So condition is %d\n", 
                //id, msg1.s_header.s_local_time, cur_bank->current, reqTime, msg1.s_header.s_local_time < reqTime ||
                // (msg1.s_header.s_local_time == reqTime && cur_bank->current > id));
                if (msg1.s_header.s_local_time < reqTime || (msg1.s_header.s_local_time == reqTime && cur_bank->current > id)) {
                    deferred[id] = 0;
                    cur_bank->lamp_time++;
                    msg1 = (Message) {
                            .s_header = {
                                    .s_magic = MESSAGE_MAGIC,
                                    .s_type  = CS_REPLY,
                                    .s_local_time = get_lamport_time(),
                                    .s_payload_len = 0
                            },
                            .s_payload = "",
                    };
                    send(cur_bank, id, &msg1);
                } else {
                    deferred[id] = 1;
                }
                break;
            case DONE:
                running_processes--;
                if(running_processes == 0)
                    printf(log_received_all_done_fmt, get_lamport_time(), cur_bank->current);
                break;
        }
    }
    return 0;
}




int release_cs(const void * self){
    bank* cur = (bank*) self;
    Message msg;

    for (local_id id = 1; id < processes_count; id++){
        if (id != cur->current && deferred[id]){
            Message message = {
                    .s_header = {
                            .s_magic = MESSAGE_MAGIC,
                            .s_local_time = ++cur->lamp_time,
                            .s_type = CS_REPLY,
                            .s_payload_len = 0,
                    }
            };
            send(cur, id, &message);
            deferred[id] = 0;
        }
    }

    cur->lamp_time++;
    msg.s_header = (MessageHeader) {
            .s_magic = MESSAGE_MAGIC,
            .s_type  = DONE,
            .s_local_time = get_lamport_time(),
            .s_payload_len = 0
    };
            if(cur->current==3) printf("Process 3 WAS THERE!!!!!!!\n");
    send_multicast(cur, &msg);
    printf(log_done_fmt, cur->current, cur->current, 0);
    return 0;
}


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
                cur_bank->lamp_time = 0;
                break;
            } else {
                cur_bank->current = PARENT_ID;
            }
        } else {
            puts("Can't create process!\n");
        }

    }
    close_unused_pipes();

    if(cur_bank->current != PARENT_ID){
        cur_bank->lamp_time++;
        Message msg = {
                .s_header =
                        { .s_magic = MESSAGE_MAGIC, .s_type = STARTED, .s_local_time = get_lamport_time(),
                        } };
        timestamp_t timestamp = get_lamport_time();
        snprintf(msg.s_payload, sizeof(msg.s_payload), log_started_fmt, timestamp, cur_bank->current, getpid(), getppid(), cur_bank->balanceHistory.s_history[timestamp].s_balance);
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        fprintf(fp, (const char *) &msg, log_started_fmt, timestamp, cur_bank->current, getpid(), getppid(), cur_bank->balanceHistory.s_history[timestamp].s_balance);
        send_multicast(&target, &msg);
    }

    for (int i = 1; i < processes_count; i++){
        Message message;
        if (i != cur_bank->current){
            receive(&target, i, &message);
            if (cur_bank->lamp_time < message.s_header.s_local_time){
                cur_bank->lamp_time = message.s_header.s_local_time;
            }
            cur_bank->lamp_time++;
        }
    }
    printf(log_received_all_started_fmt, get_lamport_time(), cur_bank->current);
    fprintf(fp, log_received_all_started_fmt, get_lamport_time(), cur_bank->current);
    
    int num_prints = cur_bank->current * 5;
    char str[128];
    if(mutexl_flag){
        if(cur_bank->current != PARENT_ID){
            request_cs(cur_bank);
        }
    } else
    for (int i = 1; i <= num_prints; ++i) {
        sprintf(str, log_loop_operation_fmt, cur_bank->current, i, num_prints);
        fprintf(fp, log_loop_operation_fmt, cur_bank->current, i, num_prints);
        print(str);
    }

    if (cur_bank->current == PARENT_ID) {
        for (int i = 1; i <= processes_count; i++) {
            waitpid(proc_pids[i], NULL, 0);
        }
    }
    fclose(fp);
    return 0;
}
