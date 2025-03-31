#include "taskQueue.h"

int taskQueue_init(taskQueue_t *taskQ)
{
    memset(taskQ, 0, sizeof(taskQueue_t));
    taskQ->pFront = taskQ->pBack = NULL;
    taskQ->size = 0;
    return 0;
}

int enqueue(taskQueue_t *taskQ, int fd)
{
    task_t* p = (task_t*)malloc(sizeof(task_t));
    p->net_fd = fd;
    p->pNext = NULL;
    if(taskQ->size == 0){
        taskQ->pFront = taskQ->pBack = p;
    }else{
        taskQ->pBack->pNext = p;
        taskQ->pBack = p;
    }
    taskQ->size++;
    return 0;
}

int dequeue(taskQueue_t *taskQ)
{
    task_t* p = taskQ->pFront;
    if(taskQ->size == 1){
        taskQ->pBack = taskQ->pFront = NULL;
    }else{
        taskQ->pFront = p->pNext;
    }
    free(p);
    taskQ->size--;
    return 0;
}
