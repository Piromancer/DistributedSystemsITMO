#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "io.h"
#include "ipc.h"
#include "pa1.h"

void close_pipes_that_dont_belong_to_us() {
    for (size_t source = 0; source < processes_count; source++) {
        for (size_t destination = 0; destination < processes_count;
             destination++) {
            if (source != current && destination != current &&
                source != destination) {
                close(output[source][destination]);
                close(input[source][destination]);
            }
            if (source == current && destination != current) {
                close(input[source][destination]);
            }
            if (destination == current && source != current) {
                close(output[source][destination]);
            }
        }
    }
}

int main(int argc, char* argv[]){
    pid_t pids[MAX_N+1];
    int num_children = 4;
    processes_count = 5;

    close_pipes_that_dont_belong_to_us();

    for (size_t source = 0; source < processes_count; source++) {
        for (size_t destination = 0; destination < processes_count; destination++) {
            if (source != destination) {
                int fd[2]; pipe(fd);
                input[source][destination] = fd[0];
                output[source][destination] = fd[1];
            }
        }
    }

    // create porcessess
    pids[PARENT_ID] = getpid();
    for (size_t id = 1; id <= num_children; id++) {
        int child_pid = fork();
        if (child_pid > 0) {
            current = PARENT_ID;
            pids[id] = child_pid;
        } else if (child_pid == 0) {
            // We are inside the child process.
            current = id;
            break;
        }
    }

    if(current!=PARENT_ID){
        Message msg = {
            .s_header =
                {
                    .s_magic = MESSAGE_MAGIC,
                    .s_type = STARTED,
                },
        };
        printf(log_started_fmt, current, getpid(), getppid());
        sprintf(msg.s_payload, log_started_fmt, current, getpid(), getppid());
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        send_multicast(NULL, &msg);
    }

    for (size_t i = 1; i <= num_children; i++) {
        Message msg;
        if (i == current) {
            continue;
        }
        printf("Proccess %d waits for started from %d\n", current, i);
        receive(NULL, i, &msg);
        printf("Proccess %d received started from %d\n", current, i);
    }
    printf(log_received_all_started_fmt, current);
    if(current!=PARENT_ID){
        Message msg = {
            .s_header =
                {
                    .s_magic = MESSAGE_MAGIC,
                    .s_type = DONE,
                },
        };
        printf(log_started_fmt, current);
        sprintf(msg.s_payload, log_done_fmt, current);
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        send_multicast(NULL, &msg);
    }
    printf(log_received_all_done_fmt, current);

    if (current == PARENT_ID) {
        for (int i = 1; i <= processes_count; i++) {
            waitpid(pids[i], NULL, 0);
        }
    }
}