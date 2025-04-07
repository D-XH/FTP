#include "cmd.h"

int send_msg(int net_fd, char* msg);
int send_data(int net_fd, data_t* data, int type);
int uid_to_name(int uid, char* name);
int user_register(int net_fd, char* username, char* password);
int user_login(int net_fd, char* username, char* password, statTree_t* loginInfo);
int change_dir(int net_fd, char* path, dir_stack_t* cwd, int uid);
int list_dir(int net_fd, char* path, dir_stack_t* cwd, int uid);
int print_work_dir(int net_fd, dir_stack_t* cwd);
int create_new_dir(int net_fd, char* dirpath, dir_stack_t* cwd, int uid);
int remove_FAndD(int net_fd, char* dirpath, dir_stack_t* cwd, int uid);
int put_file(int net_fd, char* cli_path, char* ser_path, dir_stack_t* cwd, int uid);

int process_cmd(int net_fd, statTree_t* loginInfo){
    command_t cmd;
    data_t token;
    memset(&token, 0, sizeof(token));
    memset(&cmd, 0, sizeof(cmd));
    recv(net_fd, &token.size, sizeof(token.size), 0);
    recv(net_fd, token.buf, token.size, 0);
    if(recv(net_fd, &cmd, sizeof(cmd), 0) == 0){
        return -1;
    }

    printf("===== process cmd token_size %ld op: %d =====\n", token.size, cmd.type);
    printf("===== argv[0]: %s, argv[1]: %s ====\n", cmd.argv[0], cmd.argv[1]);
    if(cmd.type == REGISTER){
        user_register(net_fd, cmd.argv[0], cmd.argv[1]);
        return 0;
    }else if(cmd.type == EXIT){
        // client exit
        return -1;
    }else if(cmd.type == LOGIN){
        user_login(net_fd, cmd.argv[0], cmd.argv[1], loginInfo);
    }else{
        tree_node_t* node = se_login_user(loginInfo, net_fd);
        if(node == NULL){
            send_msg(net_fd, "login validation failed, please relogin!");
            return 0;
        }
        char username[256] = {0};
        uid_to_name(node->uid, username);
        if(valToken(username, token.buf) == -1){
            send_msg(net_fd, "login validation failed, please relogin!");
            return 0;
        }
        if(cmd.type == CD){
            // send_msg(net_fd, "CD!");
            tree_node_t* node = se_login_user(loginInfo, net_fd);
            change_dir(net_fd, cmd.argv[0], &node->cwd, node->uid);
        }else if(cmd.type == LS){
            // send_msg(net_fd, "LS!");
            tree_node_t* node = se_login_user(loginInfo, net_fd);
            list_dir(net_fd, cmd.argv[0], &node->cwd, node->uid);
        }else if(cmd.type == PUTS){
            send_msg(net_fd, "PUTS!");
            put_file(net_fd, cmd.argv[0], cmd.argv[1], &node->cwd, node->uid);
        }else if(cmd.type == GETS){
            send_msg(net_fd, "GETS!"); 
        }else if(cmd.type == RM){
            // send_msg(net_fd, "RM!");
            remove_FAndD(net_fd, cmd.argv[0], &node->cwd, node->uid);
        }else if(cmd.type == PWD){
            // send_msg(net_fd, "PWD!");
            print_work_dir(net_fd, &node->cwd);
        }else if(cmd.type == MKDIR){
            // send_msg(net_fd, "MKDIR!");
            create_new_dir(net_fd, cmd.argv[0], &node->cwd, node->uid);
        }
    }

}
int send_data(int net_fd, data_t* data, int type){
    data_t msg_data;
    memset(&msg_data, 0, sizeof(msg_data));

    int msg_type = type;
    msg_data.size = sizeof(msg_type);
    memcpy(msg_data.buf, &msg_type, sizeof(int));
    send(net_fd, &msg_data, sizeof(msg_data.size)+msg_data.size, MSG_NOSIGNAL);

    if(send(net_fd, data, sizeof(data->size)+data->size, MSG_NOSIGNAL) == -1){
        perror("send");
    }
    return 0;
}

int send_msg(int net_fd, char* msg){
    data_t msg_data;
    memset(&msg_data, 0, sizeof(msg_data));

    int msg_type = 0;
    msg_data.size = sizeof(msg_type);
    memcpy(msg_data.buf, &msg_type, sizeof(int));
    send(net_fd, &msg_data, sizeof(msg_data.size)+msg_data.size, MSG_NOSIGNAL);

    memset(&msg_data, 0, sizeof(msg_data));
    memcpy(msg_data.buf, msg, strlen(msg));
    msg_data.size = strlen(msg);
    if(send(net_fd, &msg_data, sizeof(msg_data.size)+msg_data.size, MSG_NOSIGNAL) == -1){
        perror("send");
    }
    return 0;
}

int uid_to_name(int uid, char* name){
    MYSQL* mysql = connect_mysql();
    char query[256] = {0};
    sprintf(query, "select username from users where uid=%d and tomb=0;", uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(query_res);
    memcpy(name, row[0], strlen(row[0]));

    disconnect_mysql(mysql, query_res);
}

int filename_check(char* dir_name){
    return 0;
}

int str_replace(char* str, char old_c, char new_c){
    char* p = strchr(str, old_c);
    while(p != NULL){
        *p = new_c;
        p = strchr(str, old_c);
    }
    return 0;
}

int user_register(int net_fd, char* username, char* password){
    MYSQL* mysql = connect_mysql();

    char query[1024] = {0};
    sprintf(query, "select count(*) from users where username='%s';", username);
    int qret = mysql_query(mysql, query);
    if(qret != 0){
        printf("select username: %s\n", mysql_error(mysql));
        mysql_close(mysql);
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
        printf("crypted passwd: %s\n", crypt_passwd);

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
    }else{
        send_msg(net_fd, "username is existed");
    }
    disconnect_mysql(mysql, NULL);
    return 0;
}

int user_login(int net_fd, char* username, char* password, statTree_t* loginInfo){
    MYSQL* mysql = connect_mysql();

    char query[256] = {0};
    sprintf(query, "select uid,passwd from users where username='%s' and tomb=0;", username);
    mysql_query(mysql, query);
    MYSQL_RES* query_res = mysql_store_result(mysql);
    if(query_res == NULL){
        printf("%s\n", mysql_error(mysql));
        mysql_close(mysql);
        return -1;
    }
    uint64_t num_rows = mysql_num_rows(query_res);
    if(num_rows == 0){
        send_msg(net_fd, "login failed, please check username!");
        mysql_free_result(query_res);
        mysql_close(mysql);
        return -1;
    }
    MYSQL_ROW row = mysql_fetch_row(query_res);
    char salt[256] = {0};
    sprintf(salt, "%s9909", username);
    char* crypt_passwd = crypt(password, salt);
    if(memcmp(row[1], crypt_passwd, strlen(row[1])) == 0){
        printf("same password\n");
        data_t token_msg;
        memset(&token_msg, 0, sizeof(token_msg));
        if(getToken(username, token_msg.buf) == -1){
            printf("gettoken failed\n");
            return -1;
        }
        token_msg.size = strlen(token_msg.buf);
        send_msg(net_fd, "login sucessfully!");
        send_data(net_fd, &token_msg, 1);

        // store fd->username
        add_login_user(loginInfo, net_fd, atoi(row[0]));
    }else{
        printf("password err\n");
        send_msg(net_fd, "login failed, please check password!");
        mysql_free_result(query_res);
        mysql_close(mysql);
        return 0;
    }
    disconnect_mysql(mysql, query_res);
    return 0;
}

int filepath_ch(int net_fd, char* old_cwd, char* new_cwd, char* path, dir_stack_t* cwd){
    get_userCwd(cwd, old_cwd);
    if(path[0] == '/'){
        clearStack(cwd);
    }
    char* tok = strtok(path, "/");
    while(tok != NULL){
        if(strlen(tok) == 1 && tok[0] == '.'){
            
        }else if(strlen(tok) == 2 && memcmp(tok, "..", 2) == 0){
            if(cwd->topIdx == 0){
                set_userCwd(cwd, old_cwd);
                send_msg(net_fd, "path err");
                return 0;
            }else{
                pop(cwd);
            }
        }else{
            push(cwd, tok);
        }
        tok = strtok(NULL, "/");
    }
    get_userCwd(cwd, new_cwd);
    
    return 0;
}

int change_dir(int net_fd, char* path, dir_stack_t* cwd, int uid){
    if(strlen(path) <= 0){
        return 0;
    }

    char new_cwd[256] = {0};
    char old_cwd[256] = {0};
    filepath_ch(net_fd, old_cwd, new_cwd, path, cwd);

    MYSQL* mysql = connect_mysql();

    char query[1024] = {0};
    sprintf(query, "select * from disk where filepath='%s' and uid=%d and tomb=0;", new_cwd, uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);
    if(row_num == 0){
        send_msg(net_fd, "the path of dir is err!");
        set_userCwd(cwd, old_cwd);
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        if(memcmp(row[2], "d", 1) != 0){
            send_msg(net_fd, "path is not a directory!");
            set_userCwd(cwd, old_cwd);
        }else{
            set_userCwd(cwd, new_cwd);
        }
    }
    printf("old_cwd: '%s' -----> cur_cwd: '%s'\n", old_cwd, new_cwd);
    disconnect_mysql(mysql, query_res);
    return 0;
}

int list_dir(int net_fd, char* path, dir_stack_t* cwd, int uid){
    char new_cwd[256] = {0};
    char old_cwd[256] = {0};
    if(strlen(path) != 0){
        filepath_ch(net_fd, old_cwd, new_cwd, path, cwd);
        set_userCwd(cwd, old_cwd);
    }else{
        get_userCwd(cwd, new_cwd);
    }

    MYSQL* mysql = connect_mysql();
    char query[1024] = {0};
    sprintf(query, "select fid from disk where filepath='%s' and uid=%d and tomb=0;", new_cwd, uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);
    if(row_num == 0){
        send_msg(net_fd, "path is err");
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        mysql_free_result(query_res);
        memset(query, 0, sizeof(query));
        sprintf(query, "select * from disk where pid=%d and uid=%d and tomb=0;", atoi(row[0]), uid);
        mysql_query(mysql, query);

        query_res = mysql_store_result(mysql);
        uint64_t row_num = mysql_num_rows(query_res);
        if(row_num == 0){
            disconnect_mysql(mysql, query_res);
            return 0;
        }
        data_t data;
        memset(&data, 0, sizeof(data));
        for(int i = 0; i < row_num; i++){
            row = mysql_fetch_row(query_res);
            strcat(data.buf, row[1]);
            if(memcmp(row[2], "d", 1) == 0){
                strcat(data.buf, "/");
            }
            strcat(data.buf, " ");
        }
        data.size = strlen(data.buf);
        send_data(net_fd, &data, 2);
    }
    disconnect_mysql(mysql, query_res);
    return 0;
}

int print_work_dir(int net_fd, dir_stack_t* cwd){
    data_t data;
    memset(&data, 0, sizeof(data));
    get_userCwd(cwd, data.buf);
    data.size = strlen(data.buf);
    send_data(net_fd, &data, 2);
    return 0;
}


int create_new_dir(int net_fd, char* dirpath, dir_stack_t* cwd, int uid){
    if(strlen(dirpath) <= 0){
        return 0;
    }

    char new_cwd[256] = {0};
    char old_cwd[256] = {0};
    filepath_ch(net_fd, old_cwd, new_cwd, dirpath, cwd);
    set_userCwd(cwd, old_cwd);

    char dir_name[256] = {0};
    char* p = strrchr(new_cwd, '/'); // nerver NULL
    memcpy(dir_name, p+1, strlen(p+1));
    if(p == new_cwd){
        memset(p+1, 0, strlen(p));
    }else{
        memset(p, 0, strlen(p));
    }
    
    filename_check(dir_name);

    MYSQL* mysql = connect_mysql();
    char query[1024] = {0};
    sprintf(query, "select fid from disk where filepath='%s' and uid=%d and tomb=0;", new_cwd, uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);
    if(row_num == 0){
        send_msg(net_fd, "path is err");
    }else{
        MYSQL_ROW row = mysql_fetch_row(query_res);
        mysql_free_result(query_res);
        int pid = atoi(row[0]);

        memset(query, 0, sizeof(query));
        sprintf(query, "select fid,tomb from disk where pid=%d and filename='%s' and uid=%d;", pid, dir_name, uid);
        mysql_query(mysql, query);

        query_res = mysql_store_result(mysql);
        row_num = mysql_num_rows(query_res);
        if(row_num != 0){
            MYSQL_ROW row = mysql_fetch_row(query_res);
            mysql_free_result(query_res);
            if(atoi(row[1]) == 0){
                send_msg(net_fd, "dir has been existed!");
            }
            memset(query, 0, sizeof(query));
            sprintf(query, "update disk set tomb=0 where fid=%s;", row[0]);
            mysql_query(mysql, query);
            return 0;
        }
        mysql_free_result(query_res);

        memset(query, 0, sizeof(query));
        if(strlen(new_cwd) != 1){
            strcat(new_cwd, "/");
        }
        strcat(new_cwd, dir_name);
        sprintf(query, "insert into disk (filename, type, filepath, pid, uid) value ('%s', 'd', '%s', %d, %d);", dir_name, new_cwd, pid, uid);
        mysql_query(mysql, query);
    }
}

int remove_FAndD(int net_fd, char* dirpath, dir_stack_t* cwd, int uid){
    if(strlen(dirpath) <= 0){
        return 0;
    }
    char new_cwd[256] = {0};
    char old_cwd[256] = {0};
    filepath_ch(net_fd, old_cwd, new_cwd, dirpath, cwd);
    set_userCwd(cwd, old_cwd);

    str_replace(new_cwd, '*', '%');
    str_replace(new_cwd, '?', '-');
    // printf("rmdir %s\n", new_cwd);

    MYSQL* mysql = connect_mysql();
    char query[1024] = {0};
    sprintf(query, "select fid,type,filepath from disk where filepath like '%s' and uid=%d and tomb=0;", new_cwd, uid);
    mysql_query(mysql, query);

    MYSQL_RES* query_res = mysql_store_result(mysql);
    uint64_t row_num = mysql_num_rows(query_res);
    if(row_num == 0){
        if(net_fd != -1){
            send_msg(net_fd, "path is err");
        }
    }else{
        for(int i = 0; i < row_num; i++){
            MYSQL_ROW row = mysql_fetch_row(query_res);
            if(strlen(row[2]) == 1) continue;
            if(new_cwd[strlen(new_cwd) - 1] != '%' && memcmp(row[1], "d", 1) == 0){
                char subdir[256] = {0};
                sprintf(subdir, "%s/%%", row[2]);
                remove_FAndD(-1, subdir, cwd, uid);
            }
            if(memcmp(row[1], "f", 1) == 0){
                // delete real file
            }
            memset(query, 0, sizeof(query));
            sprintf(query, "update disk set tomb=1 where fid=%s;", row[0]);
            mysql_query(mysql, query);
        }
    }
    disconnect_mysql(mysql, query_res);
    return 0;
}

int put_file(int net_fd, char* cli_path, char* ser_path, dir_stack_t* cwd, int uid){

}