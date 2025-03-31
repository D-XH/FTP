#ifndef __WORKER_H__
#define __WORKER_H__

#include "heads.h"
#include "threadPool.h"
#include "transFile.h"

enum op{
    CD,
    LS,
    PUTS,
    GETS,
    RM,
    PWD,
    MKDIR,
    LOGIN
};

void* handler(void* arg);
int work(threadPool_t* pool, int net_fd);

#endif