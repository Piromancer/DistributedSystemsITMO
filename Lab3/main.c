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



    for (int i = 1; i <= processes_count-1; i++){
        Message message;
        if (i != cur_bank->current)
            receive(&target, i, &message);
        if (cur_bank->lamp_time < message.s_header.s_local_time){
            cur_bank->lamp_time = message.s_header.s_local_time;
            cur_bank->lamp_time++;
        }
    }
    printf(log_received_all_started_fmt, get_lamport_time(), cur_bank->current);
    fprintf(fp, log_received_all_started_fmt, get_lamport_time(), cur_bank->current);


    size_t not_ready = processes_count - 2;
    bool flag = true;
    while (flag){

        receive_any(cur_bank, &msg);
        if (cur_bank->lamp_time < msg.s_header.s_local_time){
            cur_bank->lamp_time = msg.s_header.s_local_time;
        }
        cur_bank->lamp_time++;

        MessageType messageType = msg.s_header.s_type;

        if (messageType == TRANSFER){
            TransferOrder* transferOrder = (TransferOrder*) msg.s_payload;
            timestamp_t transfer_time = msg.s_header.s_local_time;

            BalanceHistory* balanceHistory = &cur_bank->balanceHistory;
            balance_t res = 0;

            if (transferOrder->s_src == cur_bank->current){
                if (cur_bank->lamp_time < transfer_time){
                    cur_bank->lamp_time = transfer_time;
                }
                cur_bank->lamp_time++;
                timestamp_t sendTime = get_lamport_time();
                for (timestamp_t i = sendTime; i < 256; i++){
                    balanceHistory->s_history[i].s_balance -= transferOrder->s_amount;
                }

                msg.s_header.s_local_time = sendTime;

                //res = -transferOrder->s_amount;
                send(&target, transferOrder->s_dst, &msg);
                printf(log_transfer_out_fmt, sendTime, cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);
                fprintf(fp, log_transfer_out_fmt, sendTime, cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);
            } else if (transferOrder->s_dst == cur_bank->current){

                //todo
                if (cur_bank->lamp_time){};


                res = +transferOrder->s_amount;
                Message ack;
                ack.s_header = (MessageHeader) {
                    .s_magic = MESSAGE_MAGIC, .s_type = ACK, .s_local_time = transfer_time, .s_payload_len = 0
                };
                send(&target, 0, &ack);
                printf(log_transfer_out_fmt, get_physical_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_src);
                fprintf(fp, log_transfer_out_fmt, get_physical_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_src);
            }
            if (transfer_time >= balanceHistory->s_history_len){
                balanceHistory->s_history_len = transfer_time + 1;

            }
            for (timestamp_t time = transfer_time; time < 256; time++){
                balanceHistory->s_history[time].s_balance += res;
            }
        }
        if (messageType == STOP){
            flag = false;
        }
        if (messageType == DONE){
            not_ready--;
        }

    }
    msg = (Message) {
            .s_header = {
                    .s_magic = MESSAGE_MAGIC, .s_type = DONE
            }
    };
    timestamp = get_physical_time();
    printf((const char *) &msg, log_done_fmt, timestamp, cur_bank->current, cur_bank->balanceHistory.s_history[timestamp].s_balance);
    fprintf(fp, (const char *) &msg, log_done_fmt, timestamp, cur_bank->current, cur_bank->balanceHistory.s_history[timestamp].s_balance);

    msg.s_header.s_payload_len = strlen(msg.s_payload);
    send_multicast(&target, &msg);

    while (not_ready > 0){
        Message newmsg;
        receive_any(cur_bank, &newmsg);
        MessageType messageType = newmsg.s_header.s_type;

        if (messageType == TRANSFER){
            TransferOrder* transferOrder = (TransferOrder*) newmsg.s_payload;
            timestamp_t transfer_time = get_physical_time();

            BalanceHistory* balanceHistory = &cur_bank->balanceHistory;
            balance_t res = 0;

            if (transferOrder->s_src == cur_bank->current){
                res = -transferOrder->s_amount;
                send(&target, transferOrder->s_dst, &newmsg);
                printf(log_transfer_out_fmt, get_physical_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);
                fprintf(fp, log_transfer_out_fmt, get_physical_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);

            } else if (transferOrder->s_dst == cur_bank->current){
                res = +transferOrder->s_amount;
                Message ack;
                ack.s_header = (MessageHeader) {
                        .s_magic = MESSAGE_MAGIC, .s_type = ACK, .s_local_time = transfer_time, .s_payload_len = 0
                };
                send(&target, PARENT_ID, &ack);
                printf(log_transfer_out_fmt, get_physical_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);
                fprintf(fp, log_transfer_out_fmt, get_physical_time(), cur_bank->current, transferOrder->s_amount, transferOrder->s_dst);
            }
            if (transfer_time >= balanceHistory->s_history_len){
                balanceHistory->s_history_len = transfer_time + 1;

            }
            for (timestamp_t time = transfer_time; time < 256; time++){
                balanceHistory->s_history[time].s_balance += res;
            }
        }
        if (messageType == DONE){
            not_ready--;
        }
    }

    printf(log_received_all_done_fmt, get_physical_time(), cur_bank->current);
    fprintf(fp, log_received_all_done_fmt, get_physical_time(), cur_bank->current);

    cur_bank->balanceHistory.s_history_len = get_physical_time() + 1;
    int historySize = sizeof(int8_t) /*local_id */ + sizeof(uint8_t) + cur_bank->balanceHistory.s_history_len* sizeof(BalanceState);

    Message res = {
            .s_header = {
                    .s_magic = MESSAGE_MAGIC, .s_type = BALANCE_HISTORY, .s_local_time = get_physical_time(), .s_payload_len = historySize
            }
    };
    memcpy(&res.s_payload, &cur_bank->balanceHistory, historySize);
    send(cur_bank, PARENT_ID, &res);

}

void parent_start(bank* cur_bank){
    Message msg;
    for (int i = 1; i < processes_count; i++){
        if (i != cur_bank->current)
            receive(&target, i, &msg);

    }
    printf(log_received_all_started_fmt, get_physical_time(), cur_bank->current);
    fprintf(fp, log_received_all_started_fmt, get_physical_time(), cur_bank->current);

    bank_robbery(cur_bank, processes_count - 1);
    Message message = {
            .s_header = {
                    .s_magic = MESSAGE_MAGIC, .s_type = STOP, .s_payload_len = 0, .s_local_time = get_physical_time()
            }
    };
    send_multicast(&target, &message);

    for (int i = 1; i <= processes_count - 1; i++){
        if (i != cur_bank->current)
            receive(&target, i, &msg);

    }
    printf(log_received_all_done_fmt, get_physical_time(), cur_bank->current);
    fprintf(fp, log_received_all_done_fmt, get_physical_time(), cur_bank->current);

    cur_bank->allHistory.s_history_len = processes_count - 1;
    for (int i = 1; i <= processes_count - 1; i++){
        receive(&target, i, &msg);
        MessageType message_type = msg.s_header.s_type;
        if (message_type == BALANCE_HISTORY){
            BalanceHistory* childrenHistory = (BalanceHistory*) &msg.s_payload;
            cur_bank->allHistory.s_history[i-1] = *childrenHistory;
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

void transfer(void* parent_data, local_id src, local_id dst, balance_t amount)
{
    local_id* cur = parent_data;
    Message msg;
    {
        msg.s_header = (MessageHeader) { .s_local_time = get_physical_time(), .s_magic = MESSAGE_MAGIC, .s_type = TRANSFER, .s_payload_len = sizeof(TransferOrder)};
        TransferOrder order = { .s_src = src, .s_dst = dst, .s_amount = amount };
        memcpy(&msg.s_payload, &order, sizeof(TransferOrder));
        send(cur, src, &msg);
    }
    receive(cur, dst, &msg);
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
    for (int src = 0; src <= processes_count; src++) {
        for (int dst = 0; dst <= processes_count; dst++) {
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


    pids[PARENT_ID] = getpid();
    // create porcessess
    for (int id = 1; id <= children_processes_count; id++) {
        int child_pid = fork();
        if (child_pid != -1) {
            if (child_pid == 0) {
                cur_bank->current = id;
                break;
            } else {
                cur_bank->current = 0;
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
