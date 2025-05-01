#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "uthash.h"

typedef struct {
    unsigned long tid;
    time_t timestamp;
} kv_entry_t;

kv_entry_t *start_time_table = NULL;
kv_entry_t *end_time_table = NULL;
