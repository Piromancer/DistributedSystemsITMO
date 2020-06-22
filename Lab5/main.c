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
  
// Function to Create A New Node 
Node* newNode(int d, int p) 
{ 
    Node* temp = (Node*)malloc(sizeof(Node)); 
    temp->data = d; 
    temp->priority = p; 
    temp->next = NULL; 
  
    return temp; 
} 
  
// Return the value at head 
int peek(Node** head) 
{ 
    return (*head)->data; 
} 

int peekPrio(Node** head) 
{ 
    return (*head)->priority; 
} 
  
// Removes the element with the 
// highest priority form the list 
void pop(Node** head) 
{ 
    Node* temp = *head; 
    (*head) = (*head)->next; 
    free(temp); 
} 
  
void print_queue(Node** head)
{
    char res[4096] = "------------------------------------------\n";
    Node* nd = *head;
    while(nd != NULL){
        char tmp[256];
        sprintf(tmp, "Process %d with prio %d\n", nd->data, nd->priority);
        strcat(res, tmp);
        nd = nd->next;
    }
    strcat(res, "------------------------------------------\n");
    printf("%s", res);
}

// Function to push according to priority 
void push(Node** head, int d, int p) 
{ 
    Node* start = (*head); 
  
    Node* temp = newNode(d, p); 
    if ((*head)->data > d) { 
        temp->next = *head; 
        (*head) = temp; 
    } 
    else { 
  
        // Traverse the list and find a 
        // position to insert new node 
        while (start->next != NULL && 
               start->next->data < d) { 
            start = start->next; 
        }
  
        // Either at the ends of the list 
        // or at required position 
        temp->next = start->next; 
        start->next = temp; 
    } 
}
  
// Function to check is list is empty 
int isEmpty(Node** head) 
{ 
    return (*head) == NULL; 
}



int wait_queue(){
    bank* cur_bank = &target;
    int wait_reply = processes_count - 2;
    int running_processes = processes_count - 1;
    Message msg;

    while (running_processes > 0){
        //printf("%d with priority %d is next, current proccess is %d\n", peek(&cur_bank->queue), peekPrio(&cur_bank->queue), cur_bank->current);
        if(wait_reply == 0 && peek(&cur_bank->queue) == cur_bank->current){
            //print_queue(&cur_bank->queue);
            int num_prints = cur_bank->current * 5;
            char str[256];
            for (int i = 1; i <= num_prints; ++i) {
                sprintf(str, log_loop_operation_fmt, cur_bank->current, i, num_prints);
                print(str);
            }
            release_cs(cur_bank);
            pop(&cur_bank->queue);
            running_processes--;
        }
        int id = receive_any(cur_bank, &msg);
        if(cur_bank->lamp_time < msg.s_header.s_local_time) 
            cur_bank->lamp_time = msg.s_header.s_local_time;
        cur_bank->lamp_time++;
        switch(msg.s_header.s_type){
            case CS_REQUEST:
                push(&cur_bank->queue, id, msg.s_header.s_local_time);
                cur_bank->lamp_time++;
                msg = (Message) {
                    .s_header = {
                        .s_magic = MESSAGE_MAGIC,
                        .s_type  = CS_REPLY,
                        .s_local_time = get_lamport_time(),
                        .s_payload_len = 0
                    },
                    .s_payload = "",
                };
                send(cur_bank, id, &msg);
                break;
            case CS_REPLY:
                wait_reply--;
                break;
            case CS_RELEASE:
                pop(&cur_bank->queue);
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
    cur_bank->queue = newNode(cur_bank->current, get_lamport_time());
    send_multicast(cur_bank, &msg);
    return 0;
}




int release_cs(const void * self){
    bank* cur = (bank*) self;
    Message msg;
    cur->lamp_time++;
    msg.s_header = (MessageHeader) {
            .s_magic = MESSAGE_MAGIC,
            .s_type  = DONE,
            .s_local_time = get_lamport_time(),
            .s_payload_len = 0
    };
    send_multicast(cur, &msg);
    printf(log_done_fmt, cur->current, cur->current, 0);

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
            wait_queue();
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