#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "heads.h"

#include "taskQueue.h"
#include "worker.h"
#include "loginStat.h"

typedef struct threadInfo_s{
    pthread_t *pTid;
    size_t length;
}threadInfo_t;

typedef struct threadPool_s{
    // threads
    threadInfo_t threads;
    // task queue
    taskQueue_t taskQ;
    // mutex
    pthread_mutex_t mutex;
    // condition
    pthread_cond_t cond;
    // exit flag
    int exit_flag;
    // loginInfo
    statTree_t loginInfo;
}threadPool_t;

static int threadInfo_init(threadInfo_t* pThreadInfo, size_t thNum);
int threadPool_init(threadPool_t* pThreadPool, size_t thNum);

#endif