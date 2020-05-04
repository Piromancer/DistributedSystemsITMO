#include "stdio.h"
#include "io.h"
#include "ipc.h"
#include "err.h"

typedef enum {
    WRONG_DESTINATION_ID = 1,
    MESSAGE_IS_NOT_MAGIC = 2,
    WRONG_SENDER_ID = 3,
} ErrorCodes;

static int read_n_bytes(size_t pipe_cell, void *buf, int num_bytes) {
    int bytes_left = num_bytes;
    int offset = 0;
    while (bytes_left > 0) {
        int num_bytes_read = read(pipe_cell, ((char *)buf) + offset, bytes_left);
        if (num_bytes_read > 0) {
            bytes_left -= num_bytes_read;
            offset += num_bytes_read;
        } else {
            return -1;
        }
    }
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
    printf("Sending from %d\n", current);
    return 0;
}

int receive(void * self, local_id from, Message * msg){
    printf("Entered receive\n");
    if(from>=processes_count) {
        printf("entered wrong sender id - %d\n", from);
        return WRONG_SENDER_ID;
    }
    read_n_bytes(input[from][current], &msg->s_header, sizeof(MessageHeader));
    read_n_bytes(input[from][current], &msg->s_payload, msg->s_header.s_payload_len);
    if (msg->s_header.s_magic != MESSAGE_MAGIC) {
        printf("entered wrong magic\n");
        return MESSAGE_IS_NOT_MAGIC;
    }
    printf("Received message: %s\n", &msg->s_payload);
    return 0;
}

int receive_any(void * self, Message * msg){
    return fprintf(stderr, "Unused in PA1");
}