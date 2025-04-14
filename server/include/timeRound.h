#ifndef __TIME_ROUND__
#define __TIME_ROUND__

#define MAX_TIME_ROUND 10800

#include "heads.h"

typedef struct linkList_node_s{
    char* token;
    struct linkList_node_s* next;
}linkLisk_node_t;

typedef struct timeRound_s{
    size_t total_time;
    size_t cur_idx;
    linkLisk_node_t* round[MAX_TIME_ROUND];
}timeRound_t;

int timeRound_init(timeRound_t* tr, size_t time);

int timeRound_add(timeRound_t* tr, char* token);
int timeRound_del(timeRound_t* tr, size_t idx, char* token);
linkLisk_node_t* timeRound_step(timeRound_t* tr);

#endif