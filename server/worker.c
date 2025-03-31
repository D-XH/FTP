#include "worker.h"


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
        pthread_mutex_unlock(&pThreadPool->mutex);
        // work
        printf("%d start work ...\n", net_fd);
        workLoop(net_fd);
    }
    pthread_exit(NULL);
}

int loginCheck(){
    return 0;
}

int cd(char* root, char* cwd, char* dir){
    char* cd_dir = dir;
    char buf[256] = {0};
    printf("cwd: %s\n", cwd);
    if(memcmp(cd_dir, "..", 2) == 0){
        if(strlen(cwd) == 1){
            return -1;
        }
        char* p = strrchr(cwd, '/');
        memset(p, 0, strlen(p));
        sprintf(buf, "%s/%s", root, cwd);
        if(strlen(cd_dir) > 3){
            cd_dir = strchr(cd_dir, '/')+1;
            sprintf(cwd, "%s/%s", cwd, cd_dir);
        }
    }else{
        sprintf(buf, "%s/%s/%s", root, cwd, cd_dir);
        if(access(buf, F_OK) == 0){
            sprintf(cwd, "%s/%s", cwd, cd_dir);
            // ERR_CHECK(chdir(buf), -1, "chdir");
        }else{
            return -2;
        }
    }
    return 0;
}

int ls(int net_fd, char* root, char* cwd){
    char buf[256] = {0};
    sprintf(buf, "%s/%s", root, cwd);
    DIR* pdir = opendir(buf);
    if(pdir == NULL){
        perror("ls");
        return -1;
    }

    struct dirent* pDirent = readdir(pdir);
    data_t data;
    memset(&data, 0, sizeof(data));
    while(pDirent != NULL){
        strcat(data.buf, " ");
        strcat(data.buf, pDirent->d_name);
        pDirent = readdir(pdir);
    }
    data.size = strlen(data.buf);
    send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
    return 0;
}

int makedir(char* root, char* cwd, char* dir){
    char* mk_dir = dir;
    char buf[256] = {0};
    if(dir[0] == '/'){
        sprintf(buf, "%s%s",root, mk_dir);
        if(mkdir(buf, 0777) == -1){
            return -1;
        }
    }
    //....//
    return 0;
}

int rm_file(char* filepath){
    if(unlink(filepath) == -1){
        return -1;
    }
}


int workLoop(int net_fd){
    command_t cmd_data;
    data_t data;
    memset(&data, 0, sizeof(data));
    memset(&cmd_data, 0, sizeof(cmd_data));

    char root[512] = "./root_D";
    char cwd[256] = "/";
    
    umask(0);
    while(1){
        memset(&cmd_data, 0, sizeof(cmd_data));
        int rsize = recv(net_fd, &cmd_data, sizeof(cmd_data), 0);
        if(rsize == -1 || rsize == 0){
            perror("recv");
            break;
        }
        printf("rsize: %d\n", rsize);
        if(rsize == 0){
            break;
        }
        if(cmd_data.type == LOGIN){
            loginCheck();
            sprintf(root, "%s/%s", cwd, cmd_data.argv[0]);
            memcpy(data.buf, "login sucess", 12);
            data.size = 12;
            send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
        }else if(cmd_data.type == LS){
            ls(net_fd, root, cwd);
        }else if(cmd_data.type == PWD){
            char* p = cwd;
            if(strlen(cwd) > 1){
                p++;
            }
            memcpy(data.buf, p, strlen(p));
            data.size = strlen(p);
            send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
        }else if(cmd_data.type == CD){
            int sta = cd(root, cwd, cmd_data.argv[0]);
            if(sta == -2){
                memcpy(data.buf, "dir is not exist", 16);
                data.size = 16;
                send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
            }else if(sta == -1){
                memcpy(data.buf, "change failed", 13);
                data.size = 13;
                send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
            }
        }else if(cmd_data.type == MKDIR){
            if(makedir(root, cwd, cmd_data.argv[0]) == -1){
                memcpy(data.buf, "mkdir failed", 12);
                data.size = 12;
                send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
            }
        }else if (cmd_data.type == RM){
            char filepath[1024] = {0};
            sprintf(filepath, "%s/%s/%s", root, cwd, cmd_data.argv[0]);
            if(rm_file(filepath) == -1){
                memcpy(data.buf, "remove failed", 13);
                data.size = 13;
                send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
            }
        }else if(cmd_data.type == PUTS){
            char filepath[1024] = {0};
            sprintf(filepath, "%s/%s/%s", root, cwd, cmd_data.argv[1]);
            recvFile(net_fd, filepath);
        }else if(cmd_data.type == GETS){
            char filepath[1024] = {0};
            sprintf(filepath, "%s/%s/%s", root, cwd, cmd_data.argv[0]);
            trans(net_fd, filepath);
        }else if(cmd_data.type == EXIT){
            printf("exit\n");
            break;
        }
        
    }
    close(net_fd);
}



int pwd(int net_fd, char* cwd){

}