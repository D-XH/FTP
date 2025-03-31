#include "epoll.h"

int epollAdd(int epfd, int add_fd, uint32_t flag){
    struct epoll_event evt;
    evt.events = flag;
    evt.data.fd = add_fd;
    ERR_CHECK(epoll_ctl(epfd, EPOLL_CTL_ADD, add_fd, &evt), -1, "epoll add");
    return 0;
}
int epollDel(int epfd, int del_fd){
    ERR_CHECK(epoll_ctl(epfd, EPOLL_CTL_DEL, del_fd, NULL), -1, "epoll del");
    return 0;
}