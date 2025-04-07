#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/md5.h>

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
    REGISTER,
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
    }else if(memcmp(tok, "register", sizeof("register")) == 0){
        cmd_data->type = REGISTER;
    }else{
        perror("op args");
        return -1;
    }
    // int argc = 0;
    while((tok = strtok(NULL, " ")) != NULL){
        memcpy(cmd_data->argv[cmd_data->argc++], tok, strlen(tok));
        // strcat(cmd_data->argv, tok);
        // strcat(cmd_data->argv, " ");
        // argc++;
    }
    // cmd_data->len = strlen(cmd_data->argv);
    if(cmd_data->argc == 0){
        if(cmd_data->type == LOGIN || cmd_data->type == REGISTER || cmd_data->type == PUTS || cmd_data->type == GETS || cmd_data->type == RM || cmd_data->type == MKDIR){
            printf("missing operand\n");
            return -1;
        }
    }else if(cmd_data->argc == 1){
        if(cmd_data->type == LOGIN || cmd_data->type == REGISTER){
            printf("missing operand\n");
            return -1;
        }else if(cmd_data->type == PWD || cmd_data->type == EXIT){
            printf("too many args\n");
            return -1;
        }
    }else if(cmd_data->argc == 2){
        if(cmd_data->type == PUTS || cmd_data->type == GETS || cmd_data->type == RM || cmd_data->type == MKDIR || cmd_data->type == LS || cmd_data->type == PWD || cmd_data->type == EXIT){
            printf("too many args\n");
            return -1;
        }
    }else{
        printf("too many args\n");
        return -1;
    }
    return 0;
}

int recv_msg(int net_fd, char* token){
    data_t msg_data;
    memset(&msg_data, 0, sizeof(msg_data));

    int msg_type = 0;
    recvn(net_fd, &msg_data.size, sizeof(msg_data.size));
    int rsize = recvn(net_fd, msg_data.buf, msg_data.size);
    if(rsize == 0){ 
        exit(-1);
    }
    memcpy(&msg_type, msg_data.buf, msg_data.size);

    if(msg_type == 0 || msg_type == 2){
        memset(&msg_data, 0, sizeof(msg_data));
        recvn(net_fd, &msg_data.size, sizeof(msg_data.size));
        recvn(net_fd, msg_data.buf, msg_data.size);
        printf("%s\n", msg_data.buf);
    }else if(msg_type == 1){
        memset(&msg_data, 0, sizeof(msg_data));
        recvn(net_fd, &msg_data.size, sizeof(msg_data.size));
        recvn(net_fd, msg_data.buf, msg_data.size);
        memcpy(token, msg_data.buf, msg_data.size);
    }
}

void* subThread_put(void* arg){
    // net_fd, filepath
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in* addr = NULL;
    char* filepath = NULL;
    if(connect(sock_fd, (struct sockaddr*)addr, sizeof(addr)) == -1){
        perror("connect");
        close(sock_fd);
        pthread_exit(NULL);
    }

    int file_fd = open(filepath, O_RDWR);
    struct stat file_stat;
    if(fstat(file_fd, &file_stat) == -1){
        printf("put file: get file stat\n");
        close(file_fd);
        close(sock_fd);
        pthread_exit(NULL);
    }
    off_t file_size = file_stat.st_size;
    if(file_stat.st_mode & S_IFMT == S_IFDIR){
        printf("can not transfer directory!\n");
        close(file_fd);
        close(sock_fd);
        pthread_exit(NULL);
    }

    if(file_size > 100*1024*1024){
        
    }else{
        
    }
    close(file_fd);
    close(sock_fd);
    pthread_exit(NULL);
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

    char token[256] = {0};
    while(1){
        printf(">");
        fflush(stdout);
        int ready_num = epoll_wait(epfd, ready_evts, sizeof(ready_evts), -1);
        for(int i = 0; i < ready_num; i++){
            if(ready_evts[i].data.fd == sock_fd){
                // printf("socknet \n");
                printf("\b");
                fflush(stdout);
                recv_msg(sock_fd, token);
            }else if(ready_evts[i].data.fd == STDIN_FILENO){
                command_t cmd_data;
                data_t data;
                memset(&cmd_data, 0, sizeof(cmd_data));
                memset(&data, 0, sizeof(data));
                memcpy(data.buf, token, strlen(token));
                data.size = strlen(token);
                if(parse(&cmd_data) == -1){
                    break;
                }
                send(sock_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
                ssize_t ssize = send(sock_fd, &cmd_data, sizeof(cmd_data), MSG_NOSIGNAL);
                if(cmd_data.type == EXIT){
                    exit(0);
                }else if(cmd_data.type == PUTS){
                    pthread_t tid;
                    pthread_create(&tid, NULL, subThread_put, NULL);
                }else if(cmd_data.type == GETS){
                    
                }
            }
        }
    }

    return 0;
}