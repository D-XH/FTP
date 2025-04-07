#ifndef __TCP_LINK_H__
#define __TCP_LINK_H__

#include "heads.h"

typedef struct cli_info_s
{
    /* data */
    int net_fd;
    struct sockaddr_in addr;
}cli_info_t;


int tcpInit(int* sock_fd, struct sockaddr_in* addr, int n);

#endif