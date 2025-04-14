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
        // printf("line: %s\n", line);
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
                if(!S_ISDIR(st.st_mode)){exit(-1);}// 路径存在但不是目录
            }else{
                // 创建目录（设置权限为755）
                if (mkdir(store_dir, 0755) != 0) {
                    printf("fail create store_dir!\n");
                    exit(-1);
                }    
            }
        }else if(memcmp("mysql_host", line, strlen("mysql_host")) == 0){
            extern char MYSQL_HOST[64];
            memset(MYSQL_HOST, 0, sizeof(MYSQL_HOST));
            if(strlen(t) > sizeof(MYSQL_HOST)){
                perror("mysql_host err");
                exit(-1);
            }
            memcpy(MYSQL_HOST, t, strlen(t));
        }else if(memcmp("mysql_username", line, strlen("mysql_username")) == 0){
            extern char MYSQL_USERNAME[256];
            memset(MYSQL_USERNAME, 0, sizeof(MYSQL_USERNAME));
            if(strlen(t) > sizeof(MYSQL_USERNAME)){
                perror("mysql_username err");
                exit(-1);
            }
            memcpy(MYSQL_USERNAME, t, strlen(t));
        }else if(memcmp("mysql_passwd", line, strlen("mysql_passwd")) == 0){
            extern char MYSQL_PASSWD[256];
            memset(MYSQL_PASSWD, 0, sizeof(MYSQL_PASSWD));
            if(strlen(t) > sizeof(MYSQL_PASSWD)){
                perror("mysql_passwd err");
                exit(-1);
            }
            memcpy(MYSQL_PASSWD, t, strlen(t));
        }else if(memcmp("database_name", line, strlen("database_name")) == 0){
            extern char DATABASE_NAME[256];
            memset(DATABASE_NAME, 0, sizeof(DATABASE_NAME));
            if(strlen(t) > sizeof(DATABASE_NAME)){
                perror("database_name err");
                exit(-1);
            }
            memcpy(DATABASE_NAME, t, strlen(t));
        }
        line = strtok(NULL, "\n");
    }
    free(pdata);
    close(file_fd);
    return 0;
}