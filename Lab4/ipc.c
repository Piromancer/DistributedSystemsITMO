#include "stdio.h"
#include "io.h"
#include "ipc.h"
#include "err.h"
#include <unistd.h>

int send(void * self, local_id dst, const Message * msg){
    write(output[current][dst], &msg->s_header, sizeof(MessageHeader));
    write(output[current][dst], &msg->s_payload, msg->s_header.s_payload_len);
    return 0;
}

int send_multicast(void * self, const Message * msg){
    for(local_id dst=0;dst<processes_count;dst++){
        send(self,dst,msg);
    }
    return 0;
}

int receive(void * self, local_id from, Message * msg){
    read(input[from][current], &msg->s_header, sizeof(MessageHeader));
    read(input[from][current], &msg->s_payload, msg->s_header.s_payload_len);
    
    return 0;
}

int receive_any(void * self, Message * msg){
    return 0;
}
