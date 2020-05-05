#include "stdio.h"
#include "io.h"
#include "ipc.h"
#include "err.h"

typedef enum {
    WRONG_DESTINATION_ID = 1,
    MESSAGE_IS_NOT_MAGIC = 2,
    WRONG_SENDER_ID = 3,
} ErrorCodes;

static int read_n_bytes(int pipe_cell, void *buf, int num_bytes) {
    int bytes_left = num_bytes;
    int offset = 0;
    printf("Process %d Entered read_n_bytes\n", current);
    while (bytes_left > 0) {
        printf("Bytes left - %d\n", bytes_left);
        printf("%d\n", pipe_cell);
        int num_bytes_read = read(pipe_cell, ((char *)buf) + offset, bytes_left);
        if (num_bytes_read > 0) {
            bytes_left -= num_bytes_read;
            offset += num_bytes_read;
        } else {
            return -1;
        }
    }
    printf("Process %d Escaped read_n_bytes\n", current);
    return offset;
}

int send(void * self, local_id dst, const Message * msg){
    if(dst>=processes_count){
        return WRONG_DESTINATION_ID;
    } else
    if(msg->s_header.s_magic!=MESSAGE_MAGIC){
        return MESSAGE_IS_NOT_MAGIC;
    } else {
        write(output[current][dst], &msg->s_header, sizeof(MessageHeader));
        write(output[current][dst], &msg->s_payload, msg->s_header.s_payload_len);
    }
}

int send_multicast(void * self, const Message * msg){
    for(local_id dst=0;dst<processes_count;dst++){
        if(dst!=current){
            int code = send(self,dst,msg);
            if(code!=0) return code;
        }
    }
    return 0;
}

int receive(void * self, local_id from, Message * msg){
    printf("Proccess %d entered receive\n", current);
    if(from>=processes_count) {
        printf("Wrong sender\n");
        return WRONG_SENDER_ID;
    }
    read_n_bytes(input[from][current], &msg->s_header, sizeof(MessageHeader));
    printf("Process: %d Header: %s\n", current, &msg->s_header);
    read_n_bytes(input[from][current], &msg->s_payload, msg->s_header.s_payload_len);
    printf("Process %d received message from %d\n", current, from);
    if (msg->s_header.s_magic != MESSAGE_MAGIC) {
        return MESSAGE_IS_NOT_MAGIC;
    }
    return 0;
}

int receive_any(void * self, Message * msg){
    return fprintf(stderr, "Unused in PA1");
}