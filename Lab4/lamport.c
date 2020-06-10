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


int insert_into_queue(){
    node_t *node = (node_t*)malloc(sizeof(node_t));
    node->next = NULL;
    node->id = current;
    node->time = get_time();
    insert(*node);
}

int request_cs(){
    Message msg;
    increase_time();

    msg.s_header = (MessageHeader) {
        .s_magic = MESSAGE_MAGIC,
        .s_type  = CS_REQUEST,
        .s_local_time = get_time(),
        .s_payload_len = 0
    };

    send_multicast(NULL, &msg);
    insert_into_queue();
    wait_queue();
    return 0;
}

int wait_queue(){
    int num_prints = current * 5;
    int running_processes = processes_count;
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

int release_cs(){
    Message msg;
    increase_time();
    
    msg.s_header = (MessageHeader) {
        .s_magic = MESSAGE_MAGIC,
        .s_type  = CS_RELEASE,
        .s_local_time = get_time(),
        .s_payload_len = 0
    };
    
    send_multicast(NULL, &msg);
    return 0;
}