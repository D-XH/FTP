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
    LOGIN,
    EXIT
};
typedef struct command_s{
    int type;
    int argc;
    char argv[2][256];
}command_t;

void* handler(void* arg);
int workLoop(int net_fd);

#endif