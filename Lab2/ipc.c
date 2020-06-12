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
    read(input[from][cur->current], &msg->s_payload, msg->s_header.s_payload_len);
    return 0;
}

int receive_any(void* self, Message* msg){
    bank *cur = self;
    int from = cur->current;
    while (true) {
        from++;
        if (from == cur->current) continue;
        if (from >= processes_count) {
            from -= processes_count;
        }

        unsigned int flags = fcntl(input[from][cur->current], F_GETFL, 0);
        fcntl(input[from][cur->current], F_SETFL, flags | O_NONBLOCK);
        int num_bytes_read = read(input[from][cur->current], &msg->s_header, 1);
        if (num_bytes_read == -1) 
            continue;

        fcntl(input[from][cur->current], F_SETFL, flags & !O_NONBLOCK);
        read(input[from][cur->current], ((char *) &msg->s_header) + 1, sizeof(MessageHeader) - 1);
        read(input[from][cur->current], msg->s_payload, msg->s_header.s_payload_len);
        fcntl(input[from][cur->current], F_SETFL, flags | O_NONBLOCK);
        return 0;
    }
}
