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

                if (cur_bank->lamp_time < transfer_time){
                    cur_bank->lamp_time = transfer_time;
                }
                cur_bank->lamp_time++;

                res = +transferOrder->s_amount;

                timestamp_t rec_time = get_lamport_time();
                for (timestamp_t i = 0; i < rec_time; i++){
                    balanceHistory->s_history[i].s_balance_pending_in += res;
                }
                for (timestamp_t j = rec_time; j < 256; j++) {
                    balanceHistory->s_history[j].s_balance += res;
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
            balance_t res = 0;

            if (transferOrder->s_src == cur_bank->current){
                //res = -transferOrder->s_amount;

                if (cur_bank->lamp_time < transfer_time){
                    cur_bank->lamp_time = transfer_time;
                }
                cur_bank->lamp_time++;

                timestamp_t send_time = get_lamport_time();

                for (timestamp_t i = 0; i < 256; i++){
                    balanceHistory->s_history->s_balance -= transferOrder->s_amount;
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


                res = +transferOrder->s_amount;

                timestamp_t rec_time = get_lamport_time();
                for (timestamp_t i = 0; i < rec_time; i++){
                    balanceHistory->s_history[i].s_balance_pending_in += res;
                }
                for (timestamp_t j = rec_time; j < 256; j++) {
                    balanceHistory->s_history[j].s_balance += res;
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

    int historySize = sizeof(int8_t) /*local_id */ + sizeof(uint8_t) + cur_bank->balanceHistory.s_history_len* sizeof(BalanceState);

    Message res = {
            .s_header = {
                    .s_magic = MESSAGE_MAGIC, .s_type = BALANCE_HISTORY, .s_local_time = get_lamport_time(), .s_payload_len = historySize
            }
    };
    memcpy(&res.s_payload, &cur_bank->balanceHistory, historySize);
    send(cur_bank, PARENT_ID, &res);

}