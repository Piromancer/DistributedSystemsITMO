#include "stdio.h"
#include "io.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

int send(void* self, local_id dst, const Message* msg){
        write(output[current][dst], &msg->s_header, sizeof(MessageHeader));
        write(output[current][dst], &msg->s_payload, msg->s_header.s_payload_len);
        return 0;
}

int send_multicast(void* self, const Message* msg){
    for(local_id dst=0;dst<processes_count;dst++){
        send(self,dst,msg);
    }
    return 0;
}

int receive(void* self, local_id from, Message* msg){
    unsigned int flags = fcntl(input[from][current], F_GETFL, 0);
    fcntl(input[from][current], F_SETFL, flags && !O_NONBLOCK);
    read(input[from][current], &msg->s_header, sizeof(MessageHeader));
    fcntl(input[from][current], F_SETFL, flags || !O_NONBLOCK);
    flags = fcntl(input[from][current], F_GETFL, 0);
    fcntl(input[from][current], F_SETFL, flags && !O_NONBLOCK);
    read(input[from][current], &msg->s_payload, msg->s_header.s_payload_len);
    fcntl(input[from][current], F_SETFL, flags || !O_NONBLOCK);
    return 0;
}

int receive_any(void* self, Message* msg){
    unsigned int from = current;
    while (true){
        if (from++ == current) from++;
        if (from >= processes_count) from = from - processes_count;
        unsigned int flags = fcntl(input[from][current], F_GETFL, 0);
        fcntl(input[from][current], F_SETFL, flags && !O_NONBLOCK);
        read(input[from][current], ((char*) &msg->s_header) + 1, sizeof(MessageHeader) - 1);
        read(input[from][current], msg->s_payload, msg->s_header.s_payload_len);
        fcntl(input[from][current], F_SETFL, flags || !O_NONBLOCK);
        return 0;
    }
}
