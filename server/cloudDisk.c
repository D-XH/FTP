#include "heads.h"

#include "threadPool.h"
#include "parseConf.h"
#include "tcpLink.h"
#include "epoll.h"

int main(int argc, char* argv[]){
    if(argc != 2){
        perror("main args");
        return -1;
    }

    threadPool_t pool;
    threadPool_init(&pool, 16);

    int sock_fd = -1;
    struct sockaddr_in addr;
    parse(argv[1], &addr);
    tcpInit(&sock_fd, &addr, 50);

    int epfd = epoll_create(1);
    epollAdd(epfd, sock_fd, EPOLLIN);

    struct epoll_event ready_evts[1024];
    while(1){
        int ready_num = epoll_wait(epfd, ready_evts, sizeof(ready_evts), -1);
        for(int i = 0; i < ready_num; i++){
            if(ready_evts[i].data.fd == sock_fd){
                int net_fd = accept(sock_fd, NULL, NULL);
                epollAdd(epfd, net_fd, EPOLLIN|EPOLLET);
            }else{
                pthread_mutex_lock(&pool.mutex);
                enqueue(&pool.taskQ, ready_evts[i].data.fd);
                pthread_mutex_unlock(&pool.mutex);
                pthread_cond_signal(&pool.cond);
            }
        }
    }
    return 0;
}