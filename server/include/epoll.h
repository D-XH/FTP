#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "heads.h"

typedef struct net_data_s{
    int net_fd;
    int type;   // 0: 
}net_data_t;

int epollAdd_stru(int epfd, net_data_t* data, int add_fd, uint32_t flag);
int epollAdd_fd(int epfd, int add_fd, uint32_t flag);
int epollDel(int epfd, int del_fd);


#endif