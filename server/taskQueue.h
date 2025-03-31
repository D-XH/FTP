#ifndef __TASK_QUEUE_H__
#define __TASK_QUEUE_H__

#include "heads.h"

typedef struct userInfo_s{
    char user[256];
    char cwd[256];
}userInfo_t;

userInfo_t* fdToUser[1024];

typedef struct task_s{
    int net_fd;
    struct task_s* pNext;
}task_t;

typedef struct taskQueue_s{
    task_t* pFront;
    task_t* pBack;
    size_t size;
}taskQueue_t;

int taskQueue_init(taskQueue_t* taskQ);
int enqueue(taskQueue_t* taskQ, int fd);
int dequeue(taskQueue_t* taskQ);


#endif