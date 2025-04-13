#include "worker.h"

int put_work(int net_fd, MYSQL* mysql, threadPool_t* pool, tree_node_t* node);
int get_work(int net_fd, MYSQL* mysql, threadPool_t* pool, tree_node_t* node);
int recv_val(int net_fd, int* type, char* username, char* token);

void *handler(void *arg)
{
    threadPool_t* pThreadPool = (threadPool_t*)arg;
    while(1){
        pthread_mutex_lock(&pThreadPool->mutex);
        while(pThreadPool->exit_flag == 0 && pThreadPool->taskQ.size == 0){
            pthread_cond_wait(&pThreadPool->cond, &pThreadPool->mutex);
        }
        if(pThreadPool->exit_flag == 1){
            pthread_mutex_unlock(&pThreadPool->mutex);
            pthread_exit(NULL);
        }
        int net_fd = pThreadPool->taskQ.pFront->net_fd;
        dequeue(&pThreadPool->taskQ);
        MYSQL* mysql = connect_mysql();
        pthread_mutex_unlock(&pThreadPool->mutex);
        // work
        printf("%d start work ...\n", net_fd);

        int type = -1;
        char username[256] = {0};
        char token[256] = {0};
        recv_val(net_fd, &type, username, token);

        pthread_mutex_lock(&pThreadPool->mutex);
        tree_node_t* node = se_login_user(&pThreadPool->loginInfo, token);
        if(node == NULL || valToken(username, token) == -1){
            disconnect_mysql(mysql, NULL);
            pthread_mutex_unlock(&pThreadPool->mutex);
            send_resp(net_fd, 503, "val failed!", -1);
            continue;
        }
        pthread_mutex_unlock(&pThreadPool->mutex);
        send_resp(net_fd, 200, "val successful!", -1);

        if(type == 0){
            get_work(net_fd, mysql, pThreadPool, node);
        }else if(type == 1){
            put_work(net_fd, mysql, pThreadPool, node);
        }
        mysql_close(mysql);
        // put: net_fd, ser_path, cli_md5
        // get: net_fd, ser_path, cli_size
        printf("%d end work ...\n", net_fd);
    }
    pthread_exit(NULL);
}

int recv_val(int net_fd, int* type, char* username, char* token){
    recv_one_data(net_fd, type);
    recv_username_token(net_fd, username, token);
    return 0;
}

int put_work(int net_fd, MYSQL* mysql, threadPool_t* pool, tree_node_t* node){
    char cli_md5[256] = {0};
    char ser_path[256] = {0};

    recv_one_data(net_fd, cli_md5);
    recv_one_data(net_fd, ser_path);

    char file_path[256] = {0};
    extern char store_dir[];
    strcat(file_path, store_dir);
    strcat(file_path, "/");
    strcat(file_path, cli_md5);
    int file_fd = open(file_path, O_CREAT | O_TRUNC | O_RDWR, 0777);
    if(file_fd == -1){
        perror("open file");
        return -1;
    }
    printf("filepath: %s\n", file_path);

    off_t file_size = 0;
    recv_one_data(net_fd, &file_size);

    unsigned char ser_md5[256] = {0};
    trans_recv(net_fd, file_fd, file_size, ser_md5);

    if(strncmp(cli_md5, ser_md5, sizeof(ser_md5)) == 0){
        // successful
        printf("sunccess\n");

        char new_cwd[256] = {0};
        char old_cwd[256] = {0};
        pthread_mutex_lock(&pool->mutex);
        filepath_ch(node->fd, old_cwd, new_cwd, ser_path, &node->cwd);
        set_userCwd(&node->cwd, old_cwd);
        pthread_mutex_unlock(&pool->mutex);

        char query[1024] = {0};
        sprintf(query, "update disk set tomb=0 where filepath='%s' and uid=%d;", new_cwd, node->uid);
        mysql_query(mysql, query);
        close(file_fd);
        send_resp(net_fd, 200, "upload successful!", -1);
    }else{
        // failed
        printf("failed\n");
        close(file_fd);
        unlink(file_path);
        send_resp(net_fd, 502, "upload failed!", -1);
    }
    
    return 0;
}

int get_work(int net_fd, MYSQL* mysql, threadPool_t* pool, tree_node_t* node){
    char ser_path[256] = {0};
    recv_one_data(net_fd, ser_path);

    off_t file_off = 0;
    recv_one_data(net_fd, &file_off);

    char new_cwd[256] = {0};
    char old_cwd[256] = {0};
    pthread_mutex_lock(&pool->mutex);
    filepath_ch(node->fd, old_cwd, new_cwd, ser_path, &node->cwd);
    set_userCwd(&node->cwd, old_cwd);
    pthread_mutex_unlock(&pool->mutex);

    
    char query[1024] = {0};
    sprintf(query, "select fid,md5 from disk where filepath='%s' and uid=%d and tomb=0 and type='f';", new_cwd, node->uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t num_rows = mysql_num_rows(query_res);

    if(num_rows == 0){
        mysql_free_result(query_res);
        send_resp(net_fd, 501, "no such file!", -1);
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        mysql_free_result(query_res);

        char file_path[256] = {0};
        extern char store_dir[];
        strcat(file_path, store_dir);
        strcat(file_path, "/");
        strcat(file_path, row[1]);

        int file_fd = open(file_path, O_RDWR);
        trans_send(net_fd, file_fd, file_off);
        close(file_fd);

        send_resp(net_fd, 200, row[1], strlen(row[1]));
    }
}