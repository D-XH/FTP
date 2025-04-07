#include "threadPool.h"

static int threadInfo_init(threadInfo_t *pThreadInfo, size_t thNum)
{
    pThreadInfo->pTid = (pthread_t *)calloc(thNum, sizeof(pthread_t));
    pThreadInfo->length = thNum;
    return 0;
}

int threadPool_init(threadPool_t *pThreadPool, size_t thNum)
{
    // init threadInfo
    threadInfo_init(&pThreadPool->threads, thNum);
    // init task queue
    taskQueue_init(&pThreadPool->taskQ);
    // init mutex
    pthread_mutex_init(&pThreadPool->mutex, NULL);
    // init cond
    pthread_cond_init(&pThreadPool->cond, NULL);
    // init exit flag
    pThreadPool->exit_flag = 0;

    for(int i = 0; i < pThreadPool->threads.length; i++){
        pthread_create(&pThreadPool->threads.pTid[i], NULL, handler, pThreadPool);
    }

    return 0;
}