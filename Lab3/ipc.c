#include "stdio.h"
#include "io.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

int send(void* self, local_id dst, const Message* msg){
        bank *cur = self;
        //write(output[cur->current][dst], &msg->s_header, sizeof(MessageHeader));
        //write(output[cur->current][dst], &msg->s_payload, msg->s_header.s_payload_len);
        cur->lamp_time++;
        write(output[cur->current][dst], msg, msg->s_header.s_payload_len + sizeof(MessageHeader));
        return 0;
}

int send_multicast(void* self, const Message* msg){
    bank *cur_bank = self;
    for(local_id dst=0;dst<processes_count;dst++){
        if (dst != cur_bank->current) {
            int trans = send(cur_bank, dst, msg);
            if (trans > 0) return trans;
        }
    }
    return 0;
}

int receive(void* self, local_id from, Message* msg){
    bank *cur = self;
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

/*int receive_any(void * self, Message * msg) {
    bank *cur = (bank *) self;
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
            return 0;
        }
    }
}*/


int receive_any(void *self, Message *message) {
    bank *cur = (bank *) self;
    int to_whom = cur->current;
    while (true) {

        if ( ++to_whom  == cur->current) to_whom++;
        if (to_whom >= processes_count) {
            to_whom -= processes_count;
        }

        size_t src_file = input[to_whom][cur->current];
        unsigned int flags = fcntl(src_file, F_GETFL, 0);
        fcntl(src_file, F_SETFL, flags | O_NONBLOCK);
        int head_read_res = read(src_file, &message->s_header, sizeof(MessageHeader));
        switch (head_read_res) {
            case -1:
                continue;
            case 0: {
                continue;
            }
            default:
                break;
        }
        fcntl(src_file, F_SETFL, flags & !O_NONBLOCK);
        read(src_file, message->s_payload, message->s_header.s_payload_len);
        fcntl(src_file, F_SETFL, flags | O_NONBLOCK);

        return 0;
    }
}

