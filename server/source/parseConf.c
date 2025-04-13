#include "parseConf.h"

int parse_conf(char *conf_path, struct sockaddr_in* cmd_addr, struct sockaddr_in* data_addr)
{
    cmd_addr->sin_family = data_addr->sin_family = AF_INET;

    int file_fd = open(conf_path, O_RDONLY);
    struct stat file_stat;
    ERR_CHECK(fstat(file_fd, &file_stat), -1, "fstat");

    
    char* pdata = (char*)calloc(1, file_stat.st_size+1);
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
        }else if(memcmp("store_dir", line, strlen("store_dir")) == 0){
            extern char store_dir[];
            memcpy(store_dir, t, strlen(t));
            struct stat st;
            // 检查目录是否存在
            if (stat(store_dir, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    return 0;  // 目录已存在
                }
                exit(-1);  // 路径存在但不是目录
            }
            // 创建目录（设置权限为755）
            if (mkdir(store_dir, 0755) != 0) {
                printf("fail create store_dir!\n");
                exit(-1);
            }
        }
        line = strtok(NULL, "\n");
    }
    free(pdata);
    close(file_fd);
    return 0;
}