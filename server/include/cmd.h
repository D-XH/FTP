#ifndef __CMD_H__
#define __CMD_H__

#include "heads.h"
#include "jwt.h"
#include "loginStat.h"
#include "mysqlConn.h"
#include <crypt.h>

enum op{
    CD,
    LS,
    PUTS,
    GETS,
    RM,
    PWD,
    MKDIR,
    LOGIN,
    REGISTER,
    EXIT
};


typedef struct command_s{
    int type;
    int argc;
    char argv[2][256];
}command_t;

int process_cmd(int net_fd, statTree_t* loginInfo);

#endif