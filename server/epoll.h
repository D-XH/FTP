#ifndef __EPOLL_H__
#define __EPOLL_H__

#include "heads.h"

int epollAdd(int epfd, int add_fd, uint32_t flag);
int epollDel(int epfd, int del_fd);


#endif