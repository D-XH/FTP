#ifndef __CMD_H__
#define __CMD_H__

#include "heads.h"
#include "myCrypt.h"
#include "threadPool.h"
#include "mysqlConn.h"
#include "common.h"
#include "timeRound.h"
#include <crypt.h>

enum op{
    CD,
    LS,
    PUT,
    GET,
    RM,
    PWD,
    MKDIR,
    LOGIN,
    LOGOUT,
    REGISTER,
    EXIT
};


typedef struct command_s{
    int type;
    int argc;
    char argv[2][256];
}command_t;

int process_cmd(int net_fd, threadPool_t* pool, timeRound_t* time_round);

#endif