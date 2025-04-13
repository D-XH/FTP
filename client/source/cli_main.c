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
#include <openssl/md5.h>

#include "transFile.h"
#include "parseConf.h"

enum op{
    CD,
    LS,
    PUT,
    GET,
    RM,
    PWD,
    MKDIR,
    LOGIN,
    LOGOUT,
    REGISTER,
    EXIT
};

typedef struct command_s{
    int type;
    int argc;
    char argv[2][256];
}command_t;

typedef struct name_and_token_s{
    char buf[1024];
    ssize_t len;
}name_and_token_t;

typedef struct threadArg_s{
    struct sockaddr_in* data_addr;
    name_and_token_t* name_token;
    char cli_path[256];
    char ser_path[256];
}threadArg_t;

int arg_init(threadArg_t* arg, struct sockaddr_in* addr, char* cli_path, char* ser_path, name_and_token_t* name_token){
    arg->data_addr = addr;
    arg->name_token = name_token;
    memcpy(arg->cli_path, cli_path, strlen(cli_path));
    memcpy(arg->ser_path, ser_path, strlen(ser_path));
    return 0;            
}

int parse(command_t* cmd_data){
    char buf[1024] = {0};
    // ssize_t rsize = read(STDIN_FILENO, buf, sizeof(buf));
    fgets(buf, sizeof(buf), stdin);
    
    char* tok = strtok(buf, "\n");
    tok = strtok(tok, " ");
    if(tok == NULL){
        return -1;
    }
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
    }else if(memcmp(tok, "put", sizeof("put")) == 0){
        cmd_data->type = PUT;
    }else if(memcmp(tok, "get", sizeof("get")) == 0){
        cmd_data->type = GET;
    }else if(memcmp(tok, "login", sizeof("login")) == 0){
        cmd_data->type = LOGIN;
    }else if(memcmp(tok, "logout", sizeof("logout")) == 0){
        cmd_data->type = LOGOUT;
    }else if(memcmp(tok, "exit", sizeof("exit")) == 0){
        cmd_data->type = EXIT;
    }else if(memcmp(tok, "register", sizeof("register")) == 0){
        cmd_data->type = REGISTER;
    }else{
        printf("cmd err!\n");
        printf("cmd: register | login | logout | exit | ls | pwd | cd | mkdir | remove | put | get \n");
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
        if(cmd_data->type == LOGIN || cmd_data->type == REGISTER || cmd_data->type == PUT || cmd_data->type == GET || cmd_data->type == RM || cmd_data->type == MKDIR){
            printf("missing operand\n");
            return -1;
        }
    }else if(cmd_data->argc == 1){
        if(cmd_data->type == LOGIN || cmd_data->type == REGISTER){
            printf("missing operand\n");
            return -1;
        }else if(cmd_data->type == LOGIN || cmd_data->type == PWD || cmd_data->type == EXIT){
            printf("too many args\n");
            return -1;
        }
    }else if(cmd_data->argc == 2){
        if(cmd_data->type == RM || cmd_data->type == MKDIR || cmd_data->type == LS || cmd_data->type == PWD || cmd_data->type == EXIT){
            printf("too many args\n");
            return -1;
        }
    }else{
        printf("too many args\n");
        return -1;
    }
    return 0;
}

int recv_one_data(int net_fd, void* data){
    char* p = (char*)data;

    data_t _data;
    memset(&_data, 0, sizeof(_data));
    recvn(net_fd, &_data.size, sizeof(_data.size));
    ssize_t rsize = recvn(net_fd, _data.buf, _data.size);
    memcpy(p, _data.buf, _data.size);
    return _data.size;
}

int recv_resp(int sock_fd, int* code, char* resp, ssize_t* resp_size){
    recv_one_data(sock_fd, code);
    int _size = recv_one_data(sock_fd, resp);
    if(resp_size != NULL){
        *resp_size = _size;
    }
    return 0;
}

int send_one_str(int net_fd, char* str){
    data_t data;
    memset(&data, 0, sizeof(data));
    data.size = strlen(str);
    memcpy(data.buf, str, data.size);
    if(send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL) == -1){
        perror("send_one_str");
        return -1;
    }
    return 0;
}

int send_one_data(int net_fd, void* data, int size){
    char* p = (char*)data;
    data_t _data;
    memset(&_data, 0, sizeof(_data));
    _data.size = size;
    memcpy(_data.buf, p, _data.size);
    if(send(net_fd, &_data, sizeof(_data.size)+_data.size, MSG_NOSIGNAL) == -1){
        perror("send_one_str");
        return -1;
    }
    return 0;
}

int send_val(int sock_fd, int type, name_and_token_t* name_token){
    send_one_data(sock_fd, &type, sizeof(type));
    send_one_data(sock_fd, name_token->buf, name_token->len);
    return 0;
}

void md5_to_hex_string(const unsigned char *md, int len, char* hex_str) {
    for (int i = 0; i < len; i++) {
        sprintf(hex_str + i * 2, "%02x", md[i]); // 小写
    }
    hex_str[len * 2] = '\0';
}

int getMd5(unsigned char* md5, int fd){
    MD5_CTX ctx;
    MD5_Init(&ctx);
    int cnt = 0;
    while(1){
        char buf[1024] = {0};
        ssize_t rsize = read(fd, buf, sizeof(buf));
        if(rsize == 0){
            break;
        }
        MD5_Update(&ctx, buf, rsize);
    }
    lseek(fd, 0, SEEK_SET);
    unsigned char _md5[256] = {0};
    MD5_Final(_md5, &ctx);
    md5_to_hex_string(_md5, strlen(_md5), md5);
    return 0;
}

int send_file_md5(int sock_fd, char* filepath){
    unsigned char cli_md5[256] = {0};
    int file_fd = open(filepath, O_RDONLY);
    getMd5(cli_md5, file_fd);
    close(file_fd);
    send_one_str(sock_fd, cli_md5);
    return 0;
}

void* subThread_put(void* arg){
    threadArg_t* th_args = (threadArg_t*)arg;
    // net_fd, filepath
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in* addr = th_args->data_addr;
    name_and_token_t* name_token = th_args->name_token;
    char* cli_path = th_args->cli_path;
    char* ser_path = th_args->ser_path;

    
    int file_fd = open(cli_path, O_RDWR);
    if(file_fd == -1){
        perror("puts openfile");
        close(sock_fd);
        pthread_exit(NULL);
    }

    struct stat file_stat;
    if(fstat(file_fd, &file_stat) == -1){
        printf("put file: get file stat\n");
        close(file_fd);
        close(sock_fd);
        pthread_exit(NULL);
    }

    off_t file_size = file_stat.st_size;
    if(S_ISDIR(file_stat.st_mode)){
        printf("can not transfer directory!\n");
        close(file_fd);
        close(sock_fd);
        pthread_exit(NULL);
    }

    if(connect(sock_fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        close(sock_fd);
        pthread_exit(NULL);
    }

    send_val(sock_fd, 1, name_token);
    int code = 0;
    char resp[1024] = {0};
    recv_resp(sock_fd, &code, resp, NULL);
    if(code == 503){
        printf("code: %d, %s\n", code, resp);
        free(th_args);
        close(file_fd);
        close(sock_fd);
        pthread_exit(NULL);
    }

    // send md5
    char cli_md5[256] = {0};
    getMd5(cli_md5, file_fd);
    send_one_str(sock_fd, cli_md5);

    // send server filepath
    if(strlen(ser_path) <= 0){
        char* p = strrchr(cli_path, '/');
        if(p != NULL){
            p++;
        }
        memcpy(ser_path, p, strlen(p));
    }
    send_one_str(sock_fd, ser_path);

    // send file size
    send_one_data(sock_fd, &file_size, sizeof(file_size));
    printf("file size: %ld\n", file_size);

    if(file_size > 100*1024*1024){
        char* p = (char*)mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, 0);
        send(sock_fd, p, file_size, MSG_NOSIGNAL);
        munmap(p, file_size);
    }else{
        int cnt = 0;
        data_t data;
        while(cnt < file_size){
            memset(&data, 0, sizeof(data));
            ssize_t rsize = read(file_fd, data.buf, sizeof(data.buf));
            data.size = rsize;
            send(sock_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
            if(rsize == 0){
                break;
            }
            cnt += rsize;
        }
    }

    free(th_args);
    close(file_fd);
    close(sock_fd);
    pthread_exit(NULL);
}

void* subThread_get(void* arg){
    threadArg_t* th_args = (threadArg_t*)arg;

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in* addr = th_args->data_addr;
    char* cli_path = th_args->cli_path;
    char* ser_path = th_args->ser_path;
    name_and_token_t* name_token = th_args->name_token;

    
    char* filename = strrchr(ser_path, '/');
    if(filename == NULL){
        filename = ser_path;
    }else{
        filename++;
    }

    if(strlen(cli_path) <= 0){
        char cwd[256] = {0};
        getcwd(cwd, sizeof(cwd));
        sprintf(cli_path, "%s/%s", cwd, filename);
    }

    int file_fd = open(cli_path, O_RDWR | O_CREAT, 0777);
    if(file_fd == -1){
        perror("puts openfile");
        close(sock_fd);
        pthread_exit(NULL);
    }

    struct stat file_stat;
    if(fstat(file_fd, &file_stat) == -1){
        printf("put file: get file stat\n");
        close(file_fd);
        close(sock_fd);
        pthread_exit(NULL);
    }

    off_t file_size = file_stat.st_size;
    if(S_ISDIR(file_stat.st_mode)){
        file_size = 0;
        close(file_fd);
        strcat(cli_path, "/");
        strcat(cli_path, filename);
        file_fd = open(cli_path, O_RDWR);
    }

    if(connect(sock_fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        close(sock_fd);
        pthread_exit(NULL);
    }

    send_val(sock_fd, 0, name_token);

    int code = 0;
    char resp[1024] = {0};
    recv_resp(sock_fd, &code, resp, NULL);
    if(code == 503){
        printf("code: %d, %s\n", code, resp);
        free(th_args);
        close(file_fd);
        close(sock_fd);
        pthread_exit(NULL);
    }

    // send server file path
    send_one_str(sock_fd, ser_path);

    // send offset
    send_one_data(sock_fd, &file_size, sizeof(file_size));

    int cnt = 0;
    data_t data;
    lseek(file_fd, file_size, SEEK_SET);
    while(1){
        memset(&data, 0, sizeof(data));
        recvn(sock_fd, &data.size, sizeof(data.size));
        recvn(sock_fd, data.buf, data.size);
        if(data.size== 0){
            break;
        }
        write(file_fd, data.buf, data.size);
        cnt += data.size;
    }
    
    lseek(file_fd, 0, SEEK_SET);
    unsigned char cli_md5[256] = {0};
    getMd5(cli_md5, file_fd);

    unsigned char ser_md5[256] = {0};
    recv_resp(sock_fd, &code, ser_md5, NULL);

    close(file_fd);
    if(strncmp(cli_md5, ser_md5, sizeof(cli_md5)) == 0){
        printf("download successful!\n");
    }else{
        unlink(cli_path);
    }

    free(th_args);
    close(sock_fd);
    pthread_exit(NULL);
}

int _upload(int sock_fd, char* cli_path, char* ser_path, name_and_token_t* name_token, struct sockaddr_in* addr){
    threadArg_t* arg = calloc(1, sizeof(threadArg_t));
    arg_init(arg, addr, cli_path, ser_path, name_token);

    printf("start _upload!\n");
    pthread_t tid;
    pthread_create(&tid, NULL, subThread_put, arg);
    pthread_join(tid, NULL);
}

int _download(int sock_fd, char* ser_path, char* cli_path, name_and_token_t* name_token, struct sockaddr_in* addr){
    threadArg_t* arg = calloc(1, sizeof(threadArg_t));
    arg_init(arg, addr, cli_path, ser_path, name_token);

    printf("start _download!\n");
    pthread_t tid;
    pthread_create(&tid, NULL, subThread_get, arg);
    pthread_join(tid, NULL);
}
int main(int argc, char* argv[]){
    if(argc != 2){
        perror("main args");
        return -1;
    }

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in cmd_addr, data_addr;
    parse_conf(argv[1], &cmd_addr, &data_addr);

    if(connect(sock_fd, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)) == -1){
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

    name_and_token_t name_token;
    memset(&name_token, 0, sizeof(name_token));
    while(1){
        printf("disk>");
        fflush(stdout);
        int ready_num = epoll_wait(epfd, ready_evts, sizeof(ready_evts), -1);
        for(int i = 0; i < ready_num; i++){
            if(ready_evts[i].data.fd == sock_fd){
                char buf[1024] = {0};
                if(recv(sock_fd, buf, sizeof(buf), 0) == 0){
                    exit(0);
                }

            }else if(ready_evts[i].data.fd == STDIN_FILENO){
                command_t cmd_data;
                memset(&cmd_data, 0, sizeof(cmd_data));
                if(parse(&cmd_data) == -1){
                    break;
                }
                send_one_data(sock_fd, name_token.buf, name_token.len);

                ssize_t ssize = send(sock_fd, &cmd_data, sizeof(cmd_data), MSG_NOSIGNAL);
                if(cmd_data.type == PUT){
                    send_file_md5(sock_fd, cmd_data.argv[0]);
                }

                int code = 0;
                char resp[1024] = {0};
                ssize_t resp_size = 0;
                recv_resp(sock_fd, &code, resp, &resp_size);

                switch(code){
                    case 230:
                        memcpy(name_token.buf, resp, resp_size);
                        name_token.len = resp_size;
                        break;
                    case 231:
                        memset(&name_token, 0, sizeof(name_token));
                        printf("\r%s\n", resp);
                        if(cmd_data.type == EXIT) exit(0);
                        break;
                    case 350:
                        _upload(sock_fd, cmd_data.argv[0], cmd_data.argv[1], &name_token, &data_addr);
                        break;
                    case 351:
                        _download(sock_fd, cmd_data.argv[0], cmd_data.argv[1], &name_token, &data_addr);
                        break;
                    default:
                        // printf("\r%s\n", resp);
                        if(code / 100 == 5 || code == 220 || code == 257 || code == 258){
                            printf("\r%s\n", resp);
                        }
                        break;
                }
            }
        }
    }

    return 0;
}