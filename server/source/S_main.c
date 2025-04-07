#include "heads.h"

#include "threadPool.h"
#include "parseConf.h"
#include "tcpLink.h"
#include "epoll.h"
#include "cmd.h"
#include "loginStat.h"

int main(int argc, char* argv[]){
    if(argc != 2){
        perror("main args");
        return -1;
    }

    threadPool_t pool;
    threadPool_init(&pool, 3);

    int sock_fd = -1;
    struct sockaddr_in addr;
    parse(argv[1], &addr);
    tcpInit(&sock_fd, &addr, 50);

    int epfd = epoll_create(1);
    epollAdd_fd(epfd, sock_fd, EPOLLIN);

    statTree_t loginInfo;
    statTree_init(&loginInfo);

    struct epoll_event ready_evts[1024];
    while(1){
        int ready_num = epoll_wait(epfd, ready_evts, sizeof(ready_evts), -1);
        for(int i = 0; i < ready_num; i++){
            int ready_fd = ready_evts[i].data.fd;
            if(ready_fd == sock_fd){
                int net_fd = accept(sock_fd, NULL, NULL);
                epollAdd_fd(epfd, net_fd, EPOLLIN);
            }else{
                // cmd
                int pret = process_cmd(ready_fd, &loginInfo);
                if(pret == -1){
                    del_login_user(&loginInfo, ready_fd);
                    epollDel(epfd, ready_fd);
                    close(ready_fd);
                }else if(pret == 1){ // put
                    int net_fd = accept(sock_fd, NULL, NULL);
                    enqueue(&pool.taskQ, net_fd);
                }else if(pret == 2){ //get

                }
            }
        }
    }
    return 0;
}