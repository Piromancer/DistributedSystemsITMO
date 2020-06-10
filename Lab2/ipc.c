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
    for(local_id dst=0;dst<processes_count;dst++){
        send(self,dst,msg);
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
    bank* cur = (bank*) self;
    unsigned int from = cur->current;
    while (true){
        if (from++ == cur->current) from++;
        if (from >= processes_count) from = from - processes_count;
        unsigned int flags = fcntl(input[from][cur->current], F_GETFL, 0);
        fcntl(input[from][cur->current], F_SETFL, flags && !O_NONBLOCK);
        read(input[from][cur->current], ((char*) &msg->s_header) + 1, sizeof(MessageHeader) - 1);
        read(input[from][cur->current], msg->s_payload, msg->s_header.s_payload_len);
        fcntl(input[from][cur->current], F_SETFL, flags || !O_NONBLOCK);
        return 0;
    }
}
