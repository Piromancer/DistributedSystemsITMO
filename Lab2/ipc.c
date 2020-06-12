#include "stdio.h"
#include "io.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

int send(void* self, local_id dst, const Message* msg){
        bank* cur = self;
        write(output[cur->current][dst], &msg->s_header, sizeof(MessageHeader));
        write(output[cur->current][dst], &msg->s_payload, msg->s_header.s_payload_len);
        return 0;
}

int send_multicast(void* self, const Message* msg){
    bank* cur_bank = self;
    for(local_id dst=0;dst<processes_count;dst++){
        if (dst != cur_bank->current) {
            int trans = send(cur_bank, dst, msg);
            if (trans > 0) return trans;
        }
    }
    return 0;
}

int receive(void* self, local_id from, Message* msg){
    bank* cur = self;
    unsigned int flags = fcntl(input[from][cur->current], F_GETFL, 0);
    fcntl(input[from][cur->current], F_SETFL, flags && !O_NONBLOCK);
    read(input[from][cur->current], &msg->s_header, sizeof(MessageHeader));
    fcntl(input[from][cur->current], F_SETFL, flags || !O_NONBLOCK);
    flags = fcntl(input[from][cur->current], F_GETFL, 0);
    fcntl(input[from][cur->current], F_SETFL, flags && !O_NONBLOCK);
    read(input[from][cur->current], &msg->s_payload, msg->s_header.s_payload_len);
    fcntl(input[from][cur->current], F_SETFL, flags || !O_NONBLOCK);
    return 0;
}

int receive_any(void* self, Message* msg){
    bank *cur = self;
    int src = cur->current;
    while (true) {
        if (++src == cur->current) src++;
        if (src >= processes_count) {
            src -= processes_count;
        }

        size_t src_file = input[src][cur->current];
        unsigned int flags = fcntl(src_file, F_GETFL, 0);
        fcntl(src_file, F_SETFL, flags | O_NONBLOCK);
        int num_bytes_read = read(src_file, &msg->s_header, 1);
        if (num_bytes_read == -1) 
            continue;

        fcntl(src_file, F_SETFL, flags & !O_NONBLOCK);
        read(src_file, ((char *) &msg->s_header) + 1, sizeof(MessageHeader) - 1);
        read(src_file, msg->s_payload, msg->s_header.s_payload_len);
        fcntl(src_file, F_SETFL, flags | O_NONBLOCK);
        return 0;
    }
}
