#include "stdio.h"
#include "io.h"
#include "pa2345.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

/*
//send_msg
void send_msg(local_id c_id, int16_t type, const char* const log) {
    if (c_id != PARENT_ID){
        Message msg = { .s_header = { .s_magic = MESSAGE_MAGIC, .s_type = type, }, };
        printf(log, current, getpid(), getppid());
        pthread_mutex_lock (&fp);
        fprintf(fp, log, current, getpid(), getppid());
        pthread_mutex_unlock (&fp);
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
}*/




int send(void* self, local_id dst, const Message* msg){
        bank* cur = self;
        //write(output[cur->current][dst], &msg->s_header, sizeof(MessageHeader));
        //write(output[cur->current][dst], &msg->s_payload, msg->s_header.s_payload_len);
        cur->lamp_time++;
        //print("111111111111111111111111111");
        write(output[cur->current][dst], msg, msg->s_header.s_payload_len + sizeof(MessageHeader));
        return 0;
}

int send_multicast(void* self, const Message* msg){
    bank* cur_bank = self;
    for(local_id dst=0;dst<processes_count;dst++){
        if (dst != cur_bank->current) {
            send(cur_bank, dst, msg);
            /*if (trans > 0) {
                return trans;
            }*/
           // print("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
        }
    }
    return 0;
}

int receive(void* self, local_id from, Message* msg){
    bank* cur = self;
    unsigned int flags = fcntl(input[from][cur->current], F_GETFL, 0);
    fcntl(input[from][cur->current], F_SETFL, flags && !O_NONBLOCK);
    read(input[from][cur->current], &msg->s_header, sizeof(MessageHeader));
    fcntl(input[from][cur->current], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(input[from][cur->current], F_GETFL, 0);
    fcntl(input[from][cur->current], F_SETFL, flags && !O_NONBLOCK);
    read(input[from][cur->current], &msg->s_payload, msg->s_header.s_payload_len);
    fcntl(input[from][cur->current], F_SETFL, flags | O_NONBLOCK);
    return 0;
}

int receive_any(void * self, Message * msg) {
    bank* cur = self;
    while(true) {
        int nread;
        for (int from = 0; from < processes_count; from++) {
            fcntl(input[from][cur->current], F_SETFL, fcntl(input[from][cur->current], F_GETFL) | O_NONBLOCK);
            nread = read(input[from][cur->current], &msg->s_header, sizeof(MessageHeader));
            if (from == (cur->current) || nread == -1)
                continue;
            do {
                nread = read(input[from][cur->current], &msg->s_payload, msg->s_header.s_payload_len);
            } while (nread == -1);
            return cur->current;
        }
    }
}

