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
#include "lamport.h"

int front = 0;
int rear = -1;
int itemCount = 0;
static node_t queue[MAX_QUEUE];

node_t peek() {
    return queue[front];
}

bool isEmpty() {
    return itemCount == 0;
}

bool isFull() {
    return itemCount == MAX_QUEUE;
}

int size() {
    return itemCount;
}

void insert(node_t data) {

    if(!isFull()) {

        if(rear == MAX_QUEUE-1) {
            rear = -1;
        }

        queue[++rear] = data;
        itemCount++;
    }
}

node_t removeData() {
    node_t data = queue[front++];

    if(front == MAX_QUEUE) {
        front = 0;
    }

    itemCount--;
    return data;
}


void insert_into_queue(){
    bank* bank1 = &target;
    node_t *node = (node_t*)malloc(sizeof(node_t));
    node->next = NULL;
    node->id = bank1->current;
    node->time = get_lamport_time();
    insert(*node);
}



int wait_queue(){
    bank* cur_bank = &target;
    int wait_reply = processes_count - 1;
    int running_processes = processes_count - 1;
    while (running_processes > 0){
        Message msg;
        receive_any(NULL, &msg);
        if (cur_bank->lamp_time < msg.s_header.s_local_time){
            cur_bank->lamp_time = msg.s_header.s_local_time;
        }
        cur_bank->lamp_time++;
        switch (msg.s_header.s_type){
            case CS_REQUEST: {
                break;
            }
            case CS_REPLY: {
                wait_reply--;
                if (wait_reply == 0){
                    char str[128];
                    int num_prints = cur_bank->current * 5;

                    for (int i = 1; i <= num_prints; ++i) {
                        memset(str, 0, sizeof(str));
                        if (mutexl_flag){
                            sprintf(str, log_loop_operation_fmt, cur_bank->current, i, num_prints);
                            request_cs(cur_bank);
                            print(str);
                            release_cs(cur_bank);
                            print("yep");

                        } else {
                            sprintf(str, log_loop_operation_fmt, cur_bank->current, i, num_prints);
                            print(str);
                            print("nope");
                        }
                    }
                    Message message;

                    cur_bank->lamp_time++;
                    message.s_header = (MessageHeader) {
                            .s_magic = MESSAGE_MAGIC,
                            .s_type  = CS_RELEASE,
                            .s_local_time = get_lamport_time(),
                            .s_payload_len = 0
                    };
                    send_multicast(NULL, &message);

                    cur_bank->lamp_time++;
                    message.s_header = (MessageHeader) {
                            .s_magic = MESSAGE_MAGIC,
                            .s_type  = DONE,
                            .s_local_time = get_lamport_time(),
                            .s_payload_len = 0
                    };
                }
                break;
            }
            case CS_RELEASE: {
                removeData();
                break;
            }
            case DONE: {
                running_processes--;
                break;
            }
        }
    }
    // while (wait_reply != 0 || (process->queue->len && process->queue->start->id != process->self_id) ) {
    //     int id;
    //     while ((id = receive_any((void*)process, &message)) < 0);
    //     set_lamport_time(message.s_header.s_local_time);
    //     switch (message.s_header.s_type) {
    //         case CS_REQUEST: {
    //             insert_into_queue(process->queue, make_node(id, message.s_header.s_local_time));
    //             increase_lamport_time();
    //             message.s_header.s_type = CS_REPLY;
    //             message.s_header.s_local_time = get_lamport_time();
    //             send((void*)process, id, &message);
    //             break;
    //         }
    //         case CS_REPLY: {
    //             wait_reply--;
    //             break;
    //         }
    //         case CS_RELEASE: {
    //             del_first_of_queue(process->queue);
    //             break;
    //         }
    //         case DONE: {
    //             process->running_processes--;
    //             break;
    //         }

    //     }
    // }
    return 0;
}


int request_cs(const void * self){
    bank* cur_bank = (bank*) self;

    cur_bank->lamp_time++;
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
    insert_into_queue();
    wait_queue();
    return 0;
}




int release_cs(const void * self){
    bank* cur = (bank*) self;
    Message msg;
    cur->lamp_time++;

    msg.s_header = (MessageHeader) {
            .s_magic = MESSAGE_MAGIC,
            .s_type  = CS_RELEASE,
            .s_local_time = get_lamport_time(),
            .s_payload_len = 0
    };

    send_multicast(cur, &msg);
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
        /*if (mutexl_flag){
            puts("AOAOAOOAOAOAOAOAOAOAOO YES");
        } else puts ("YAYAYAYAYAYAY NO");*/
        if (mutexl_flag){
            sprintf(str, log_loop_operation_fmt, cur_bank->current, i, num_prints);
            print("BEFORE CS");
            request_cs(cur_bank);
            print(str);
            release_cs(cur_bank);
            print("YEP");
        } else {
            sprintf(str, log_loop_operation_fmt, cur_bank->current, i, num_prints);
            print(str);
            //print("NOPE");
        }

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
