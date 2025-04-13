#include "cmd.h"

int user_register(int net_fd, char* username, char* password, MYSQL* mysql);
int user_login(int net_fd, char* username, char* password, MYSQL* mysql, threadPool_t* pool);
int change_dir(int net_fd, char* path, MYSQL* mysql, threadPool_t* pool, tree_node_t* node);
int list_dir(int net_fd, char* path, MYSQL* mysql, threadPool_t* pool, tree_node_t* node);
int print_work_dir(int net_fd, threadPool_t* pool, tree_node_t* node);
int create_new_dir(int net_fd, char* dirpath, MYSQL* mysql, threadPool_t* pool, tree_node_t* node);
int remove_FAndD(int net_fd, char* dirpath, MYSQL* mysql, threadPool_t* pool, tree_node_t* node);
int put_file(int net_fd, char* cli_path, char* ser_path, MYSQL* mysql, threadPool_t* pool, tree_node_t* node);
int get_file(int net_fd, char* cli_path, char* ser_path, MYSQL* mysql, threadPool_t* pool, tree_node_t* node);
int user_logout(int net_fd, threadPool_t* pool, char* token);

int process_cmd(int net_fd, threadPool_t* pool){
    char username[256] = {0};
    char token[256] = {0};
    recv_username_token(net_fd, username, token);
    command_t cmd_msg;
    memset(&cmd_msg, 0, sizeof(cmd_msg));
    if(recv(net_fd, &cmd_msg, sizeof(cmd_msg), 0) == 0){
        return -1;
    }

    printf("===== process cmd username: %s token_size %ld =====\n", username, strlen(token));
    printf("===== op: %d argv[0]: %s, argv[1]: %s ====\n", cmd_msg.type, cmd_msg.argv[0], cmd_msg.argv[1]);

    MYSQL* mysql = connect_mysql();
    if(cmd_msg.type == REGISTER){
        user_register(net_fd, cmd_msg.argv[0], cmd_msg.argv[1], mysql);
        return 0;
    }else if(cmd_msg.type == EXIT){
        // client exit
        del_login_user(&pool->loginInfo, token);
        send_resp(net_fd, 231, "exit successful!", -1);
        return -1;
    }else if(cmd_msg.type == LOGIN){
        pthread_mutex_lock(&pool->mutex);
        tree_node_t* node = se_login_user(&pool->loginInfo, token);
        pthread_mutex_unlock(&pool->mutex);
        if(node != NULL && valToken(username, token) == 0){
            send_resp(net_fd, 504, "have logined, logout firsetly!", -1);
        }else{
            user_login(net_fd, cmd_msg.argv[0], cmd_msg.argv[1], mysql, pool);
        }
    }else{
        pthread_mutex_lock(&pool->mutex);
        tree_node_t* node = se_login_user(&pool->loginInfo, token);
        pthread_mutex_unlock(&pool->mutex);
        if(node == NULL || valToken(username, token) == -1){
            send_resp(net_fd, 530, "login validation failed, please relogin!", -1);
            disconnect_mysql(mysql, NULL);
            return 0;
        }
        if(cmd_msg.type == CD){
            change_dir(net_fd, cmd_msg.argv[0], mysql, pool, node);
        }else if(cmd_msg.type == LS){
            list_dir(net_fd, cmd_msg.argv[0], mysql, pool, node);
        }else if(cmd_msg.type == PUT){
            put_file(net_fd, cmd_msg.argv[0], cmd_msg.argv[1],mysql, pool, node);
        }else if(cmd_msg.type == GET){
            get_file(net_fd, cmd_msg.argv[1], cmd_msg.argv[0], mysql, pool, node);
        }else if(cmd_msg.type == RM){
            remove_FAndD(net_fd, cmd_msg.argv[0], mysql, pool, node);
        }else if(cmd_msg.type == PWD){
            print_work_dir(net_fd, pool, node);
        }else if(cmd_msg.type == MKDIR){
            create_new_dir(net_fd, cmd_msg.argv[0], mysql, pool, node);
        }else if(cmd_msg.type == LOGOUT){
            user_logout(net_fd, pool, token);
        }
    }
    disconnect_mysql(mysql, NULL);

}

int user_register(int net_fd, char* username, char* password, MYSQL* mysql){
    char query[1024] = {0};
    sprintf(query, "select count(*) from users where username='%s';", username);
    int qret = mysql_query(mysql, query);
    if(qret != 0){
        printf("select username: %s\n", mysql_error(mysql));
        return -1;
    }
    MYSQL_RES* query_res = mysql_store_result(mysql);
    ERR_CHECK(query_res, NULL, mysql_error(mysql));
    MYSQL_ROW row = mysql_fetch_row(query_res);
    mysql_free_result(query_res);

    if(atoi(row[0]) == 0){
        char salt[256] = {0};
        sprintf(salt, "%s9909", username);
        char* crypt_passwd = crypt(password, salt);

        memset(query, 0, sizeof(query));
        sprintf(query, "insert into users (username, passwd) value ('%s', '%s');", username, crypt_passwd);
        qret = mysql_query(mysql, query);
        if(qret != 0){
            printf("insert err: %s\n", mysql_error(mysql));
        }

        memset(query, 0, sizeof(query));
        sprintf(query, "select uid from users where username='%s' and tomb=0;", username);
        mysql_query(mysql, query);
        query_res = mysql_store_result(mysql);
        MYSQL_ROW row = mysql_fetch_row(query_res);
        mysql_free_result(query_res);

        memset(query, 0, sizeof(query));
        sprintf(query, "insert into disk (filename, type, filepath, uid) value ('/', 'd', '/', %d);", atoi(row[0]));
        mysql_query(mysql, query);

        send_resp(net_fd, 220, "register successful!", -1);
    }else{
        send_resp(net_fd, 530, "register failed!", -1);
    }

    return 0;
}

int user_login(int net_fd, char* username, char* password, MYSQL* mysql, threadPool_t* pool){
    char query[256] = {0};
    sprintf(query, "select uid,passwd from users where username='%s' and tomb=0;", username);
    mysql_query(mysql, query);
    MYSQL_RES* query_res = mysql_store_result(mysql);
    if(query_res == NULL){
        printf("%s\n", mysql_error(mysql));
        return -1;
    }
    uint64_t num_rows = mysql_num_rows(query_res);
    if(num_rows == 0){
        send_resp(net_fd, 530, "login failed, please check username!", -1);
        mysql_free_result(query_res);
        return -1;
    }
    MYSQL_ROW row = mysql_fetch_row(query_res);
    char salt[256] = {0};
    sprintf(salt, "%s9909", username);
    char* crypt_passwd = crypt(password, salt);
    
    if(memcmp(row[1], crypt_passwd, strlen(row[1])) == 0){
        char token[256] = {0};
        if(getToken(username, token) == -1){
            printf("gettoken failed\n");
            return -1;
        }
        char name_and_token[1024] = {0};
        sprintf(name_and_token, "%s %s", username, token);
        char encrypt_name_and_token[1024] = {0};
        char userkey[16] = "deng";
        int en_size = encrypt_str(name_and_token, encrypt_name_and_token, userkey);

        printf("%s login in, nANDt: %ld\n", username, strlen(encrypt_name_and_token));
        send_resp(net_fd, 230, encrypt_name_and_token, en_size);

        // store fd->username
        pthread_mutex_lock(&pool->mutex);
        add_login_user(&pool->loginInfo, net_fd, atoi(row[0]), token);
        pthread_mutex_unlock(&pool->mutex);
    }else{
        send_resp(net_fd, 530, "login failed, please check password!", -1);
        mysql_free_result(query_res);
        return 0;
    }
    mysql_free_result(query_res);
    return 0;
}



int change_dir(int net_fd, char* path, MYSQL* mysql, threadPool_t* pool, tree_node_t* node){
    if(strlen(path) <= 0){
        return 0;
    }

    char new_cwd[256] = {0};
    char old_cwd[256] = {0};
    pthread_mutex_lock(&pool->mutex);
    filepath_ch(net_fd, old_cwd, new_cwd, path, &node->cwd);
    pthread_mutex_unlock(&pool->mutex);
    

    char query[1024] = {0};
    sprintf(query, "select * from disk where filepath='%s' and uid=%d and tomb=0;", new_cwd, node->uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);

    pthread_mutex_lock(&pool->mutex);
    if(row_num == 0){
        send_resp(net_fd, 501, "no such directory!", -1);
        set_userCwd(&node->cwd, old_cwd);
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        if(memcmp(row[2], "d", 1) != 0){
            send_resp(net_fd, 501, "path is not a directory!", -1);
            set_userCwd(&node->cwd, old_cwd);
        }else{
            send_resp(net_fd, 200, "cd successful!", -1);
            set_userCwd(&node->cwd, new_cwd);
        }
    }
    pthread_mutex_unlock(&pool->mutex);

    printf("old_cwd: '%s' -----> cur_cwd: '%s'\n", old_cwd, new_cwd);
    return 0;
}

int list_dir(int net_fd, char* path, MYSQL* mysql, threadPool_t* pool, tree_node_t* node){
    char new_cwd[256] = {0};
    char old_cwd[256] = {0};
    pthread_mutex_lock(&pool->mutex);
    if(strlen(path) != 0){
        filepath_ch(net_fd, old_cwd, new_cwd, path, &node->cwd);
        set_userCwd(&node->cwd, old_cwd);
    }else{
        get_userCwd(&node->cwd, new_cwd);
    }
    pthread_mutex_unlock(&pool->mutex);

    char query[1024] = {0};
    sprintf(query, "select fid from disk where filepath='%s' and uid=%d and tomb=0;", new_cwd, node->uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);
    if(row_num == 0){
        send_resp(net_fd, 501, "no such file or directory!", -1);
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        mysql_free_result(query_res);
        memset(query, 0, sizeof(query));
        sprintf(query, "select * from disk where pid=%d and uid=%d and tomb=0;", atoi(row[0]), node->uid);
        mysql_query(mysql, query);

        query_res = mysql_store_result(mysql);
        uint64_t row_num = mysql_num_rows(query_res);
        if(row_num == 0){
            send_resp(net_fd, 258, "empty directory!", -1);
            mysql_free_result(query_res);
            return 0;
        }

        char ls_res[1024] = {0};
        for(int i = 0; i < row_num; i++){
            row = mysql_fetch_row(query_res);
            strcat(ls_res, row[1]);
            if(memcmp(row[2], "d", 1) == 0){
                strcat(ls_res, "/");
            }
            strcat(ls_res, " ");
        }
        send_resp(net_fd, 258, ls_res, strlen(ls_res));
    }
    mysql_free_result(query_res);
    return 0;
}

int print_work_dir(int net_fd, threadPool_t* pool, tree_node_t* node){
    char cwd[1024] = {0};

    pthread_mutex_lock(&pool->mutex);
    get_userCwd(&node->cwd, cwd);
    pthread_mutex_unlock(&pool->mutex);

    send_resp(net_fd, 257, cwd, strlen(cwd));
    return 0;
}


int create_new_dir(int net_fd, char* dirpath, MYSQL* mysql, threadPool_t* pool, tree_node_t* node){
    if(strlen(dirpath) <= 0){
        return 0;
    }

    char new_cwd[256] = {0};
    char old_cwd[256] = {0};

    pthread_mutex_lock(&pool->mutex);
    filepath_ch(net_fd, old_cwd, new_cwd, dirpath, &node->cwd);
    set_userCwd(&node->cwd, old_cwd);
    pthread_mutex_unlock(&pool->mutex);

    char dir_name[256] = {0};
    char* p = strrchr(new_cwd, '/'); // nerver NULL
    memcpy(dir_name, p+1, strlen(p+1));
    if(p == new_cwd){
        memset(p+1, 0, strlen(p));
    }else{
        memset(p, 0, strlen(p));
    }
    
    

    char query[1024] = {0};
    sprintf(query, "select fid from disk where filepath='%s' and uid=%d and tomb=0;", new_cwd, node->uid);
    mysql_query(mysql, query);
    
    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);
    if(row_num == 0){
        send_resp(net_fd, 501, "no such file or directory!", -1);
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        mysql_free_result(query_res);
        int pid = atoi(row[0]);

        memset(query, 0, sizeof(query));
        sprintf(query, "select fid,tomb from disk where pid=%d and filename='%s' and uid=%d;", pid, dir_name, node->uid);
        mysql_query(mysql, query);

        query_res = mysql_store_result(mysql);
        row_num = mysql_num_rows(query_res);
        if(row_num != 0){
            MYSQL_ROW row = mysql_fetch_row(query_res);
            mysql_free_result(query_res);
            if(atoi(row[1]) == 0){
                send_resp(net_fd, 501, "dir has been existed!", -1);
            }
            memset(query, 0, sizeof(query));
            sprintf(query, "update disk set tomb=0 where fid=%s;", row[0]);
            mysql_query(mysql, query);
            send_resp(net_fd, 200, "mkdir successful!", -1);
            return 0;
        }
        mysql_free_result(query_res);

        memset(query, 0, sizeof(query));
        if(strlen(new_cwd) != 1){
            strcat(new_cwd, "/");
        }
        strcat(new_cwd, dir_name);
        sprintf(query, "insert into disk (filename, type, filepath, pid, uid) value ('%s', 'd', '%s', %d, %d);", dir_name, new_cwd, pid, node->uid);
        mysql_query(mysql, query);
        send_resp(net_fd, 200, "mkdir successful!", -1);
    }
    mysql_free_result(query_res);
    return 0;
}

int remove_FAndD(int net_fd, char* dirpath, MYSQL* mysql, threadPool_t* pool, tree_node_t* node){
    if(strlen(dirpath) <= 0){
        return 0;
    }
    char new_cwd[256] = {0};
    char old_cwd[256] = {0};

    pthread_mutex_lock(&pool->mutex);
    filepath_ch(net_fd, old_cwd, new_cwd, dirpath, &node->cwd);
    set_userCwd(&node->cwd, old_cwd);
    pthread_mutex_unlock(&pool->mutex);

    str_replace(new_cwd, '*', '%');
    str_replace(new_cwd, '?', '-');
    // printf("rmdir %s\n", new_cwd);

    char query[1024] = {0};
    sprintf(query, "select fid,type,filepath,md5 from disk where filepath like '%s' and uid=%d and tomb=0;", new_cwd, node->uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);
    if(row_num == 0){
        if(net_fd != -1){
            send_resp(net_fd, 501, "no such file or directory!", -1);
        }
    }else{
        for(int i = 0; i < row_num; i++){
            MYSQL_ROW row = mysql_fetch_row(query_res);
            if(strlen(row[2]) == 1) continue;
            if(new_cwd[strlen(new_cwd) - 1] != '%' && memcmp(row[1], "d", 1) == 0){
                char subdir[256] = {0};
                sprintf(subdir, "%s/%%", row[2]);
                remove_FAndD(-1, subdir, mysql, pool, node);
            }
            memset(query, 0, sizeof(query));
            sprintf(query, "update disk set tomb=1 where fid=%s;", row[0]);
            mysql_query(mysql, query);

            if(memcmp(row[1], "f", 1) == 0){
                // delete real file
                memset(query, 0, sizeof(query));
                sprintf(query, "select fid from disk where md5='%s' and tomb=0;", row[3]);
                mysql_query(mysql, query);

                MYSQL_RES* new_sel = mysql_store_result(mysql);
                uint64_t new_num = mysql_num_rows(new_sel);
                mysql_free_result(new_sel);
                if(new_num == 0){
                    char file_path[1024] = {0};
                    extern char store_dir[];
                    strcat(file_path, store_dir);
                    strcat(file_path, "/");
                    strcat(file_path, row[3]);
                    printf("rm %s\n", file_path);
                    unlink(file_path);
                }

            }
        }
    }
    if(net_fd != -1){
        send_resp(net_fd, 200, "reovme successful!", -1);
    }
    mysql_free_result(query_res);
    return 0;
}

int second_trans(MYSQL* mysql, char* cli_md5, int uid, char* filename, char* store_dir, char* filepath, int store_dir_fid, int* fid){
    char query[256] = {0};
    sprintf(query, "select fid,tomb from disk where md5='%s' and tomb=0 and type='f';", cli_md5);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);
    mysql_free_result(query_res);

    int tomb = 0;
    if(row_num == 0){
        tomb = 1;
    }

    memset(query, 0, sizeof(query));
    sprintf(query, "select fid,type,tomb from disk where filepath='%s' and uid=%d;", filepath, uid);
    mysql_query(mysql, query);
    query_res = mysql_store_result(mysql);
    row_num = mysql_num_rows(query_res);
    if(row_num == 0){
        memset(query, 0, sizeof(query));
        sprintf(query, "insert into disk (filename, type, filepath, pid, uid, md5, tomb) value ('%s', 'f', '%s', %d, %d, '%s', %d);", filename, filepath, store_dir_fid, uid, cli_md5, tomb);
        mysql_query(mysql, query);
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        if(memcmp(row[1], "d", 1) == 0){
            mysql_free_result(query_res);
            return -2;
        }
        if(atoi(row[2]) == 1){
            memset(query, 0, sizeof(query));
            sprintf(query, "update disk set tomb=0,md5='%s' where fid=%d;", cli_md5, atoi(row[0]));
            mysql_query(mysql, query);
        }else{
            mysql_free_result(query_res);
            return -2;
        }
    }
    mysql_free_result(query_res);
    
    return tomb;
}

int check_dir(MYSQL* mysql, char* dir, int uid, int* dir_fid){
    char query[256] = {0};
    sprintf(query, "select fid from disk where filepath='%s' and type='d' and uid=%d and tomb=0;", dir, uid);
    mysql_query(mysql, query);
    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t num_row = mysql_num_rows(query_res);
    if(num_row == 0){
        mysql_free_result(query_res);
        return -1;
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        mysql_free_result(query_res);
        *dir_fid = atoi(row[0]);
        return 0;
    }
}



int put_file(int net_fd, char* cli_path, char* ser_path, MYSQL* mysql, threadPool_t* pool, tree_node_t* node){
    if(strlen(cli_path) <= 0){
        return 0;
    }

    char store_dir[256] = {0};
    int store_dir_fid = -1;
    char filename[256] = {0};
    char filepath[256] = {0};

    // recv md5
    unsigned char cli_md5[256] = {0};
    recv_one_data(net_fd, cli_md5);
    printf("cli_md5: %s\n", cli_md5);

    pthread_mutex_lock(&pool->mutex);
    if(strlen(ser_path) <= 0){
        char* p = strrchr(cli_path, '/');
        sprintf(filename, "%s", p == NULL?cli_path:p+1);
        get_userCwd(&node->cwd, store_dir);

        char new_cwd[256] = {0};
        char old_cwd[256] = {0};
        filepath_ch(net_fd, old_cwd, new_cwd, filename, &node->cwd);
        set_userCwd(&node->cwd, old_cwd);
        memcpy(filepath, new_cwd, strlen(new_cwd));
    }else{
        char new_cwd[256] = {0};
        char old_cwd[256] = {0};
        filepath_ch(net_fd, old_cwd, new_cwd, ser_path, &node->cwd);
        set_userCwd(&node->cwd, old_cwd);
        memcpy(filepath, new_cwd, strlen(new_cwd));

        char* p = strrchr(new_cwd, '/');
        sprintf(filename, "%s", p+1);
        if(p == new_cwd){
            strcat(store_dir, "/");
        }else{
            memcpy(store_dir, new_cwd, p-new_cwd);
        }
    }
    if(check_dir(mysql, store_dir, node->uid, &store_dir_fid) == -1){
        printf("store: %s\n", store_dir);
        send_resp(net_fd, 501, "no such directory!", -1);
        return 0;
    }
    pthread_mutex_unlock(&pool->mutex);
    printf("serpath: %s\n", filepath);

    int fid = -1;
    int st_ret = second_trans(mysql, cli_md5, node->uid, filename, store_dir, filepath, store_dir_fid, &fid);
    if(st_ret == -2){
        send_resp(net_fd, 501, "no such file or directory!", -1);
    }else if(st_ret == 0){
        printf("second trans successful!\n");
        send_resp(net_fd, 200, "upload successfully!", -1);
    }else{
        send_resp(net_fd, 350, "can't implement second_trans", -1);
    }
    return 0;
}

int get_file(int net_fd, char* cli_path, char* ser_path, MYSQL* mysql, threadPool_t* pool, tree_node_t* node){
    if(strlen(ser_path) <= 0){
        return 0;
    }

    char new_cwd[256] = {0};
    char old_cwd[256] = {0};
    pthread_mutex_lock(&pool->mutex);
    filepath_ch(net_fd, old_cwd, new_cwd, ser_path, &node->cwd);
    set_userCwd(&node->cwd, old_cwd);
    pthread_mutex_unlock(&pool->mutex);

    char query[1024] = {0};
    sprintf(query, "select fid from disk where filepath='%s' and uid=%d and tomb=0 and type='f';", new_cwd, node->uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t num_rows = mysql_num_rows(query_res);
    mysql_free_result(query_res);

    if(num_rows == 0){
        send_resp(net_fd, 501, "no such file!", -1);
    }else{
        send_resp(net_fd, 351, "continue get file!", -1);
    }
    return 0;
}

int user_logout(int net_fd, threadPool_t* pool, char* token){
    pthread_mutex_lock(&pool->mutex);
    del_login_user(&pool->loginInfo, token);
    pthread_mutex_unlock(&pool->mutex);
    send_resp(net_fd, 231, "logout successful!", -1);
    return 0;
}