#include "parseConf.h"

int parse(char *conf_path, struct sockaddr_in* addr)
{
    addr->sin_family = AF_INET;

    int file_fd = open(conf_path, O_RDONLY);
    struct stat file_stat;
    ERR_CHECK(fstat(file_fd, &file_stat), -1, "fstat");

    
    char* pdata = (char*)calloc(1, file_stat.st_size);
    read(file_fd, pdata, file_stat.st_size);
    char* p = pdata;

    char* line = strtok(p, "\n");
    while(line != NULL){
        char* t = strchr(line, '=')+1;
        if(memcmp("port", line, strlen("port")) == 0){
            addr->sin_port = htons(atoi(t));
        }else if(memcmp("ip", line, strlen("ip")) == 0){
            addr->sin_addr.s_addr = inet_addr(t);
        }
        line = strtok(NULL, "\n");
    }
    free(pdata);
    close(file_fd);
    return 0;
}