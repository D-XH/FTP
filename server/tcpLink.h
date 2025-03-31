#ifndef __TCP_LINK_H__
#define __TCP_LINK_H__

#include "heads.h"

int tcpInit(int* sock_fd, struct sockaddr_in* addr, int n);

#endif