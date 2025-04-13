#include "heads.h"

#include "threadPool.h"
#include "parseConf.h"
#include "tcpLink.h"
#include "epoll.h"
#include "cmd.h"
#include "loginStat.h"

char store_dir[256] = "./disk";

int main(int argc, char* argv[]){
    if(argc != 2){
        perror("main args");
        return -1;
    }

    threadPool_t pool;
    threadPool_init(&pool, 3);

    int sock_fd_cmd = -1;
    int sock_fd_data = -1;
    struct sockaddr_in cmd_addr, data_addr;
    parse_conf(argv[1], &cmd_addr, &data_addr);
    tcpInit(&sock_fd_cmd, &cmd_addr, 50);
    tcpInit(&sock_fd_data, &data_addr, 50);
    printf("cmd: %d, data: %d\n", ntohs(cmd_addr.sin_port), ntohs(data_addr.sin_port));

    int epfd = epoll_create(1);
    epollAdd_fd(epfd, sock_fd_cmd, EPOLLIN);
    epollAdd_fd(epfd, sock_fd_data, EPOLLIN);

    struct epoll_event ready_evts[1024];
    while(1){
        int ready_num = epoll_wait(epfd, ready_evts, sizeof(ready_evts), -1);
        for(int i = 0; i < ready_num; i++){
            int ready_fd = ready_evts[i].data.fd;
            if(ready_fd == sock_fd_cmd){
                int net_fd = accept(sock_fd_cmd, NULL, NULL);
                epollAdd_fd(epfd, net_fd, EPOLLIN);
            }else if(ready_fd == sock_fd_data){
                int net_fd = accept(sock_fd_data, NULL, NULL);
                printf("acc\n");
                pthread_mutex_lock(&pool.mutex);
                enqueue(&pool.taskQ, net_fd);
                pthread_mutex_unlock(&pool.mutex);
                pthread_cond_signal(&pool.cond);
            }else{
                // cmd
                int pret = process_cmd(ready_fd, &pool);
                if(pret == -1){
                    epollDel(epfd, ready_fd);
                    close(ready_fd);
                }
            }
        }
    }
    return 0;
}