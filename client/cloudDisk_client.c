#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "transFile.h"

enum op{
    CD,
    LS,
    PUTS,
    GETS,
    RM,
    PWD,
    MKDIR,
    LOGIN,
    EXIT
};

typedef struct command_s{
    int type;
    int argc;
    char argv[2][256];
}command_t;


int parse(command_t* cmd_data){
    char buf[1024] = {0};
    // ssize_t rsize = read(STDIN_FILENO, buf, sizeof(buf));
    fgets(buf, sizeof(buf), stdin);
    
    char* tok = strtok(buf, "\n");
    tok = strtok(tok, " ");
    // char* tok = strtok(buf, " ");
    // printf("op: %s\n", tok);
    if(memcmp(tok, "ls", sizeof("ls")) == 0){
        cmd_data->type = LS;
    }else if(memcmp(tok, "pwd", sizeof("pwd")) == 0){
        cmd_data->type = PWD;
    }else if(memcmp(tok, "cd", sizeof("cd")) == 0){
        cmd_data->type = CD;
    }else if(memcmp(tok, "remove", sizeof("remove")) == 0){
        cmd_data->type = RM;
    }else if(memcmp(tok, "mkdir", sizeof("mkdir")) == 0){
        cmd_data->type = MKDIR;
    }else if(memcmp(tok, "puts", sizeof("puts")) == 0){
        cmd_data->type = PUTS;
    }else if(memcmp(tok, "gets", sizeof("gets")) == 0){
        cmd_data->type = GETS;
    }else if(memcmp(tok, "login", sizeof("login")) == 0){
        cmd_data->type = LOGIN;
    }else if(memcmp(tok, "exit", sizeof("exit")) == 0){
        cmd_data->type = EXIT;
    }else{
        perror("op args");
        return -1;
    }
    while((tok = strtok(NULL, " ")) != NULL){
        memcpy(cmd_data->argv[cmd_data->argc++], tok, strlen(tok));
    }
    // for(int i = 0; i < cmd_data->argc; i++){
    //     printf("i: %d, argv: %s\n", i, cmd_data->argv[i]);
    // }
    if(cmd_data->type == LS || cmd_data->type == PWD || cmd_data->type == EXIT){
        if(cmd_data->argc != 0){
            printf("argc err\n");
            return -1;
        }
    }else if(cmd_data->type == LOGIN || cmd_data->type == PUTS || cmd_data->type == GETS){
        if(cmd_data->argc != 2){
            printf("input login username passwd\n");
            return -1;
        }
    }else{
        if(cmd_data->argc != 1){
            printf("argc err\n");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]){
    if(argc != 3){
        perror("main args");
        return -1;
    }

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    if(connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("connect");
        return -1;
    }
    
    int epfd = epoll_create(1);
    struct epoll_event evt, ready_evts[1024];
    evt.events = EPOLLIN;
    evt.data.fd = sock_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &evt);

    evt.events = EPOLLIN;
    evt.data.fd = STDIN_FILENO;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &evt);

    while(1){
        printf(">");
        fflush(stdout);
        int ready_num = epoll_wait(epfd, ready_evts, sizeof(ready_evts), -1);
        for(int i = 0; i < ready_num; i++){
            if(ready_evts[i].data.fd == sock_fd){
                data_t data;
                memset(&data, 0, sizeof(data));
                ssize_t rsize = recv(sock_fd, &data.size, sizeof(data.size),0);
                if(rsize == 0){
                    exit(-1);
                }
                recv(sock_fd, data.buf, data.size,0);
                printf("%s\n", data.buf);
            }else if(ready_evts[i].data.fd == STDIN_FILENO){
                command_t cmd_data;
                memset(&cmd_data, 0, sizeof(cmd_data));
                if(parse(&cmd_data) == -1){
                    break;
                }
                ssize_t ssize = send(sock_fd, &cmd_data, sizeof(cmd_data), MSG_NOSIGNAL);
                if(cmd_data.type == EXIT){
                    exit(0);
                }else if(cmd_data.type == PUTS){
                    trans(sock_fd, cmd_data.argv[0]);
                }else if(cmd_data.type == GETS){
                    recvFile(sock_fd, cmd_data.argv[1]);
                }
            }
        }
    }

    return 0;
}