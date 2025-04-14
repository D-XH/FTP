#include "parseConf.h"

int parse_conf(char *conf_path, struct sockaddr_in* cmd_addr, struct sockaddr_in* data_addr)
{
    cmd_addr->sin_family = data_addr->sin_family = AF_INET;

    int file_fd = open(conf_path, O_RDONLY);
    struct stat file_stat;
    fstat(file_fd, &file_stat);

    
    char* pdata = (char*)calloc(1, file_stat.st_size);
    read(file_fd, pdata, file_stat.st_size);
    char* p = pdata;

    char* line = strtok(p, "\n");
    while(line != NULL){
        char* t = strchr(line, '=')+1;
        if(memcmp("cmd_port", line, strlen("cmd_port")) == 0){
            cmd_addr->sin_port = htons(atoi(t));
        }else if(memcmp("data_port", line, strlen("data_port")) == 0){
            data_addr->sin_port = htons(atoi(t));
        }else if(memcmp("ip", line, strlen("ip")) == 0){
            cmd_addr->sin_addr.s_addr = inet_addr(t);
            data_addr->sin_addr.s_addr = inet_addr(t);
        }
        line = strtok(NULL, "\n");
    }
    free(pdata);
    close(file_fd);
    return 0;
}