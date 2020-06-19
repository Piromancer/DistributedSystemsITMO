#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
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
    bank* cur_bank = &target;
    Message msg;
    cur_bank->lamp_time++;

    msg.s_header = (MessageHeader) {
            .s_magic = MESSAGE_MAGIC,
            .s_type  = CS_REQUEST,
            .s_local_time = get_lamport_time(),
            .s_payload_len = 0
    };
    send_multicast(NULL, &msg);
    insert_into_queue();
    wait_queue();
    return 0;
}




int release_cs(const void * self){
    bank* cur = &target;
    Message msg;
    cur->lamp_time++;
    
    msg.s_header = (MessageHeader) {
        .s_magic = MESSAGE_MAGIC,
        .s_type  = CS_RELEASE,
        .s_local_time = get_lamport_time(),
        .s_payload_len = 0
    };
    
    send_multicast(NULL, &msg);
    return 0;
}
