#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>

void close_pipes();

void display_usage(){
    puts( "-p X, where X - number of processes 0..10 \n" );
}


void send_msg(current c_id, <type?> type) {
    if (cur_id != PARENT_ID){
        Message msg = { .s_header = { .s_magic = MESSAGE_MAGIC, .s_type = type, }, };
        sprintf(msg.s_payload, log_started_fmt, cur_id, getpid(), getppid());
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        send_multicast(NULL, &msg);
        //todo log
    }
}

void receive_msg(current c_id, unsigned int child_proc_num){
    for (int i = 1; i <= child_proc_num; i++){
        Message msg;
        if (i != c_id){
            receive(NULL,i,&msg);
        }
    }
}




int main( int argc, char *argv[] ) {

    current cur_id;
    unsigned int proc_num, children_proc_num;
    int r_pipe[10][10];
    int w_pipe[10][10];

    int opt = 0;
    static const char *optString = "p:?";
    opt = getopt( argc, argv, optString );
    while( opt != -1 ) {
        switch( opt ) {
            case 'p':
                children_proc_num = strtoul(optarg, NULL, 10);
                break;
            case '?':
                display_usage();
                return 1;
            default:
                return 1;
        }
        opt = getopt( argc, argv, optString );
    }
    proc_num = children_proc_num + 1;


    //pipes
    for (int src = 0; src < proc_num; src++) {
        for (int dst = 0; dst < proc_num; dst++) {
            if (src != dst){
                int fld[2];
                pipe(fld);
                r_pipe[src][dst] = fld[0];
                w_pipe[src][dst] = fld[1];
                //todo logs
            }
        }
    }


    pid_t proc_pids[children_proc_num];
    proc_pids[PARENT_ID] = getpid();

    //children processes
    for (int id = 1; id <= children_proc_num; id++) {
        int child_pid = fork();
        if (child_pid != -1) {
            if (child_pid == 0) {
                cur_id = id;
                break;
            } else {
                cur_id = PARENT_ID;
            }
        } else {
            puts("Can't create process!\n");
        }

    }

    close_pipes();

    /*send STARTED
    if (cur_id != PARENT_ID){
        Message msg = { .s_header = { .s_magic = MESSAGE_MAGIC, .s_type = STARTED, }, };
        sprintf(msg.s_payload, log_started_fmt, cur_id, getpid(), getppid());
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        send_multicast(NULL, &msg);
        //todo log
    }*/

    send_msg(cur_id, STARTED);

    /*receive STARTED
    for (int i = 0; i <= children_proc_num - 1; i++){
        Message msg;
        if (i != cur_id){
            receive(NULL,i,&msg);
        }
    }*/
    //todo log

    receive_msg(cur_id, children_proc_num);

    /*send DONE
    if (cur_id != PARENT_ID){
        Message msg = { .s_header = { .s_magic = MESSAGE_MAGIC, .s_type = DONE, }, };
        sprintf(msg.s_payload, log_started_fmt, cur_id);
        msg.s_header.s_payload_len = strlen(msg.s_payload);
        send_multicast(NULL, &msg);
        //todo log
    }*/

    send_msg(cur_id, DONE);

    /*receive DONE
    for (int i = 0; i <= children_proc_num - 1; i++){
        Message msg;
        if (i != cur_id){
            receive(NULL,i,&msg);
        }
    }*/
    //todo log

    receive_msg(cur_id, children_proc_num);


    if (cur_id == PARENT_ID){
        for(int i = 1; i<= children_proc_num; i++){
            waitpid(proc_pids[i], NULL, 0);
        }
    }


    return 0;
}
