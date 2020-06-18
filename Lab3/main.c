#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdbool.h>

#include "io.h"
#include "ipc.h"
#include "pa2345.h"
#include "banking.h"


timestamp_t get_lamport_time(){
    return target.lamp_time;
}

void child_start (bank* cur_bank, balance_t init_bal){
    cur_bank->balanceHistory.s_id = cur_bank->current;
    cur_bank->balanceHistory.s_history_len = 1;
    for (timestamp_t timestamp = 0; timestamp < 256; timestamp++){
        cur_bank->balanceHistory.s_history[timestamp] = (BalanceState) { .s_balance = init_bal, .s_balance_pending_in = 0, .s_time = timestamp };
    }

    cur_bank->lamp_time++;

    Message msg = {
            .s_header =
                    { .s_magic = MESSAGE_MAGIC, .s_type = STARTED, .s_local_time = get_lamport_time()
                    } };

    timestamp_t timestamp = get_lamport_time();

    msg.s_header.s_payload_len = strlen(msg.s_payload);

    printf((const char *) &msg, log_started_fmt, timestamp, cur_bank->current, getpid(), getppid(), cur_bank->balanceHistory.s_history[timestamp].s_balance);
    fprintf(fp, (const char *) &msg, log_started_fmt, timestamp, cur_bank->current, getpid(), getppid(), cur_bank->balanceHistory.s_history[timestamp].s_balance);

    send_multicast(&target, &msg);



    for (int i = 1; i < processes_count; i++){
        Message message;
        if (i != cur_bank->current){
            receive(&target, i, &message);
            if (cur_bank->lamp_time < message.s_header.s_local_time){
                cur_bank->lamp_time = message.s_header.s_local_time;
            }
            cur_bank->lamp_time++;
        }
    }
    printf(log_received_all_started_fmt, get_lamport_time(), cur_bank->current);
    fprintf(fp, log_received_all_started_fmt, get_lamport_time(), cur_bank->current);


    size_t not_ready = processes_count - 2;
    bool flag = true;
    while (flag){
        Message msg9k;
        receive_any(cur_bank, &msg9k);
        if (cur_bank->lamp_time < msg9k.s_header.s_local_time){
            cur_bank->lamp_time = msg9k.s_header.s_local_time;
        }
        cur_bank->lamp_time++;

        MessageType messageType = msg9k.s_header.s_type;

        if (messageType == TRANSFER){
            TransferOrder* transferOrder = (TransferOrder*) msg9k.s_payload;
            timestamp_t transfer_time = msg9k.s_header.s_local_time;

            BalanceHistory* balanceHistory = &cur_bank->balanceHistory;
            balance_t money = 0;

            if (transferOrder->s_src == cur_bank->current){
                if (cur_bank->lamp_time < transfer_time){
                    cur_bank->lamp_time = transfer_time;
                }
                cur_bank->lamp_time++;
                timestamp_t sendTime = get_lamport_time();
                for (timestamp_t i = sendTime; i < 256; i++){
                    balanceHistory->s_history[i].s_balance -= transferOrder->s_amount;
                }

                msg9k.s_header.s_local_time = sendTime;

                //money = transferOrder->s_amount;
                send(&target, transferOrder->s_dst, &msg9k);
                printf(log_transfer_out_fmt, sendTime, cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);
                fprintf(fp, log_transfer_out_fmt, sendTime, cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);
            } else if (transferOrder->s_dst == cur_bank->current){

                if (cur_bank->lamp_time < transfer_time){
                    cur_bank->lamp_time = transfer_time;
                }
                cur_bank->lamp_time++;

                money = transferOrder->s_amount;

                timestamp_t rec_time = get_lamport_time();
                for (timestamp_t i = transfer_time; i < rec_time; i++){
                    balanceHistory->s_history[i].s_balance_pending_in += money;
                }
                for (timestamp_t i = rec_time; i < 256; i++){
                    balanceHistory->s_history[i].s_balance += money;
                }


                Message ack;
                ack.s_header = (MessageHeader) {
                    .s_magic = MESSAGE_MAGIC, .s_type = ACK, .s_local_time = get_lamport_time(), .s_payload_len = 0
                };
                send(&target, PARENT_ID, &ack);
                printf(log_transfer_out_fmt, get_lamport_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_src);
                fprintf(fp, log_transfer_out_fmt, get_lamport_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_src);
            }
        }
        if (messageType == STOP){
            flag = false;
            if (cur_bank->lamp_time < msg.s_header.s_local_time){
                cur_bank->lamp_time = msg.s_header.s_local_time;
            }
        }
        if (messageType == DONE){
            not_ready--;
            if (cur_bank->lamp_time < msg.s_header.s_local_time){
                cur_bank->lamp_time = msg.s_header.s_local_time;
            }
        }

    }
    cur_bank->lamp_time++;
    msg = (Message) {
            .s_header = {
                    .s_magic = MESSAGE_MAGIC, .s_type = DONE, .s_local_time = get_lamport_time()
            }
    };
    timestamp = get_lamport_time();
    printf((const char *) &msg, log_done_fmt, timestamp, cur_bank->current, cur_bank->balanceHistory.s_history[timestamp].s_balance);
    fprintf(fp, (const char *) &msg, log_done_fmt, timestamp, cur_bank->current, cur_bank->balanceHistory.s_history[timestamp].s_balance);

    msg.s_header.s_payload_len = strlen(msg.s_payload);
    send_multicast(&target, &msg);

    while (not_ready > 0){
        Message newmsg;
        receive_any(cur_bank, &newmsg);

        if (cur_bank->lamp_time < newmsg.s_header.s_local_time){
            cur_bank->lamp_time = newmsg.s_header.s_local_time;
        }
        cur_bank->lamp_time++;

        MessageType messageType = newmsg.s_header.s_type;

        if (messageType == TRANSFER){
            TransferOrder* transferOrder = (TransferOrder*) newmsg.s_payload;
            timestamp_t transfer_time = newmsg.s_header.s_local_time;

            BalanceHistory* balanceHistory = &cur_bank->balanceHistory;
            balance_t money = 0;

            if (transferOrder->s_src == cur_bank->current){
                //money = transferOrder->s_amount;


                if (cur_bank->lamp_time < transfer_time){
                    cur_bank->lamp_time = transfer_time;
                }
                cur_bank->lamp_time++;

                timestamp_t send_time = get_lamport_time();

                for (timestamp_t i = send_time; i < 256; i++){
                    balanceHistory->s_history[i]->s_balance -= transferOrder->s_amount;
                }
                newmsg.s_header.s_local_time = send_time;
                send(&target, transferOrder->s_dst, &newmsg);
                printf(log_transfer_out_fmt, send_time, cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);
                fprintf(fp, log_transfer_out_fmt, send_time, cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);

            } else if (transferOrder->s_dst == cur_bank->current){


                if (cur_bank->lamp_time < transfer_time){
                    cur_bank->lamp_time = transfer_time;
                }
                cur_bank->lamp_time++;

                money = transferOrder->s_amount;

                timestamp_t rec_time = get_lamport_time();
                for (timestamp_t i = transfer_time; i < rec_time; i++){
                    balanceHistory->s_history[i].s_balance_pending_in += money;
                }
                for (timestamp_t i = rec_time; i < 256; i++){
                    balanceHistory->s_history[i].s_balance += money;
                }



                Message ack;
                ack.s_header = (MessageHeader) {
                        .s_magic = MESSAGE_MAGIC, .s_type = ACK, .s_local_time = get_lamport_time(), .s_payload_len = 0
                };
                send(&target, PARENT_ID, &ack);
                printf(log_transfer_out_fmt, get_lamport_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_src);
                fprintf(fp, log_transfer_out_fmt, get_lamport_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_src);
            }
        }
        if (messageType == DONE){
            not_ready--;
            if (cur_bank->lamp_time < newmsg.s_header.s_local_time){
                cur_bank->lamp_time = newmsg.s_header.s_local_time;
            }
        }
    }

    printf(log_received_all_done_fmt, get_lamport_time(), cur_bank->current);
    fprintf(fp, log_received_all_done_fmt, get_lamport_time(), cur_bank->current);
    cur_bank->lamp_time++;

    cur_bank->balanceHistory.s_history_len = get_lamport_time() + 1;
    
    int historySize = sizeof(uint8_t) + sizeof(uint8_t) + cur_bank->balanceHistory.s_history_len* sizeof(BalanceState);

    Message result = {
            .s_header = {
                    .s_magic = MESSAGE_MAGIC, .s_type = BALANCE_HISTORY, .s_local_time = get_lamport_time(), .s_payload_len = historySize
            }
    };
    memcpy(&result.s_payload, &cur_bank->balanceHistory, historySize);
    send(cur_bank, PARENT_ID, &result);

}

void parent_start(bank* cur_bank){

    for (int i = 1; i < processes_count; i++){
        Message msg;
        if (i != cur_bank->current) {
            receive(&target, i, &msg);
            if (cur_bank->lamp_time < msg.s_header.s_local_time){
                cur_bank->lamp_time = msg.s_header.s_local_time;
            }
            cur_bank->lamp_time++;
        }
    }
    printf(log_received_all_started_fmt, get_lamport_time(), cur_bank->current);
    fprintf(fp, log_received_all_started_fmt, get_lamport_time(), cur_bank->current);

    bank_robbery(cur_bank, processes_count - 1);
    cur_bank->lamp_time++;
    Message message = {
            .s_header = {
                    .s_magic = MESSAGE_MAGIC, .s_type = STOP, .s_payload_len = 0, .s_local_time = get_lamport_time()
            }
    };
    send_multicast(&target, &message);

    for (int i = 1; i < processes_count; i++){
        Message newmsg;
        if (i != cur_bank->current) {
            receive(&target, i, &newmsg);
            if (cur_bank->lamp_time < newmsg.s_header.s_local_time) {
                cur_bank->lamp_time = newmsg.s_header.s_local_time;

            }
            cur_bank->lamp_time++;
        }
    }
    printf(log_received_all_done_fmt, get_lamport_time(), cur_bank->current);
    fprintf(fp, log_received_all_done_fmt, get_lamport_time(), cur_bank->current);

    cur_bank->allHistory.s_history_len = processes_count - 1;
    for (int i = 1; i < processes_count; i++){
        Message newmsg;
        receive(&target, i, &newmsg);
        MessageType message_type = newmsg.s_header.s_type;
        if (message_type == BALANCE_HISTORY){
            BalanceHistory* childrenHistory = (BalanceHistory*) &newmsg.s_payload;
            cur_bank->allHistory.s_history[i-1] = *childrenHistory;
            if (cur_bank->lamp_time < message.s_header.s_local_time){
                cur_bank->lamp_time = message.s_header.s_local_time;

            }
            cur_bank->lamp_time++;
        }
    }

    for (int i = 1; i < processes_count; i++){
        waitpid(pids[i], NULL, 0);
    }
    print_history(&cur_bank->allHistory);
}

void close_unused_pipes(){
    bank* cur_bank = &target;
    for (unsigned int src = 0; src < processes_count; src++) {
        for (unsigned int dst = 0; dst < processes_count; dst++) {
            if (src != cur_bank->current && dst != cur_bank->current && src != dst) {
                close(output[src][dst]);
                close(input[src][dst]);
            }
            if (src == cur_bank->current && dst != cur_bank->current) {
                close(input[src][dst]);
            }
            if (dst == cur_bank->current && src != cur_bank->current) {
                close(output[src][dst]);
            }
        }
    }
}


void display_usage(){
    puts( "-p X Y..Y, where X - number of processes 0..10, Y - start balance for each process" );
}



void transfer(void* parent_data, local_id src, local_id dst, balance_t amount){
    bank* cur = parent_data;
    Message msg;
    {
        cur->lamp_time++;
        msg.s_header = (MessageHeader) { .s_local_time = get_lamport_time(), .s_magic = MESSAGE_MAGIC, .s_type = TRANSFER, .s_payload_len = sizeof(TransferOrder)};
        TransferOrder order = { .s_src = src, .s_dst = dst, .s_amount = amount };
        memcpy(&msg.s_payload, &order, sizeof(TransferOrder));
        send(cur, src, &msg);
    }
    receive(cur, dst, &msg);
    if (msg.s_header.s_type == ACK){
        if (cur->lamp_time < msg.s_header.s_local_time){
            cur->lamp_time = msg.s_header.s_local_time;

        }
        cur->lamp_time++;
    }
}

int main( int argc, char* argv[] ){
    bank* cur_bank = &target;
    unsigned int children_processes_count;
    fp = fopen("events.log", "w");
    int opt = 0;
    static const char* optString = "p:?";
    opt = getopt(argc, argv, optString);
    while(opt != -1) {
        switch(opt) {
            case 'p':
                children_processes_count = strtoul(optarg, NULL, 10);
                for (int i = 0; i < children_processes_count; i++){
                    banks[i+1] = strtoul(argv[i+3], NULL,10);
                }
                break;
            case '?':
                display_usage();
                return 1;
            default:
                puts( "Error - unknown error" );
                return 1;
        }
        opt = getopt( argc, argv, optString );
    }

    processes_count = children_processes_count + 1;
    //pipes
    for (int src = 0; src < processes_count; src++) {
        for (int dst = 0; dst < processes_count; dst++) {
            if (src != dst){
                int fld[2];
                pipe(fld);
                for (int k = 0; k <= 1; k++){
                    unsigned int flags = fcntl( fld[src], F_GETFL, 0);
                    fcntl(fld[k], F_SETFL, flags | O_NONBLOCK);
                }
                input[src][dst] = fld[0];
                output[src][dst] = fld[1];
            }
        }
    }
    cur_bank->lamp_time = 0;


    pids[PARENT_ID] = getpid();
    // create porcessess
    for (int id = 1; id <= children_processes_count; id++) {
        int child_pid = fork();
        if (child_pid != -1) {
            if (child_pid == 0) {
                cur_bank->current = id;
                break;
            } else {
                cur_bank->current = PARENT_ID;
                pids[id] = child_pid;
            }
        } else {
            puts("Can't create process!");
        }

    }
    close_unused_pipes();

    if (cur_bank->current == PARENT_ID) {
        parent_start(cur_bank);
    } else {
        child_start(cur_bank, banks[cur_bank->current]);
    }
    fclose(fp);
    return 0;
}
