#include "banking.h"

static timestamp_t global_time = 0;

void set_time(timestamp_t time) {
    global_time = (time > global_time ? time : global_time) + 1;
}

timestamp_t get_time(void) {
    return global_time;
}

void increase_time(void) {
    ++global_time;
}
