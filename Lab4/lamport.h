#include "pa2345.h"
#include "io.h"
#include "ipc.h"

#define MAX_QUEUE 2000000

typedef struct _node_t {
    struct _node_t *next;
    local_id id;
    timestamp_t time;
} node_t;

