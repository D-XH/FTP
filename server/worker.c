#include "worker.h"

char root[] = "./root";

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
        work(pThreadPool, net_fd);
    }
    pthread_exit(NULL);
}

int loginCheck(){
    return 0;
}

int login(threadPool_t* pool, int net_fd, int argc, char* argv[]){
    // validation
    // ...
    //
    pthread_mutex_lock(&pool->mutex);
    userInfo_t* user = fdToUser[net_fd];
    if(user != NULL){
        send(net_fd, "have logined", 12, MSG_NOSIGNAL);
        pthread_mutex_unlock(&pool->mutex);
        return 0;
    }
    pthread_mutex_unlock(&pool->mutex);
    loginCheck();
    userInfo_t* p = (userInfo_t*)malloc(sizeof(userInfo_t));
    memcpy(p->user, argv[0], strlen(argv[0]));
    memcpy(p->cwd, "/", 1);
    pthread_mutex_lock(&pool->mutex);
    fdToUser[net_fd] = p;
    pthread_mutex_unlock(&pool->mutex);
}

int cd(threadPool_t* pool, int net_fd, int argc, char* argv[]){
    pthread_mutex_lock(&pool->mutex);
    userInfo_t* user = fdToUser[net_fd];
    if(user == NULL){
        send(net_fd, "not login", 9, MSG_NOSIGNAL);
        pthread_mutex_unlock(&pool->mutex);
        return 0;
    }
    pthread_mutex_unlock(&pool->mutex);
    if(argc != 1){
        send(net_fd, "cd path err", 11, MSG_NOSIGNAL);
        return 0;
    }
    char path[256] = {0};
    strcat(path, root);
    pthread_mutex_lock(&pool->mutex);
    strcat(path, user->cwd);
    pthread_mutex_unlock(&pool->mutex);
    strcat(path, argv[0]);
    if(access(path, F_OK) != 0){
        send(net_fd, "cd path err", 11, MSG_NOSIGNAL);
        return 0;
    }
    chdir(path);
    pthread_mutex_lock(&pool->mutex);
    strcat(user->cwd, "/");
    strcat(user->cwd, argv[0]);
    pthread_mutex_unlock(&pool->mutex);
}

int work(threadPool_t* pool, int net_fd){
    data_t data;
    memset(&data, 0, sizeof(data));
    int rsize = recv(net_fd, &data.size, sizeof(data.size), 0);
    rsize = recv(net_fd, data.buf, data.size, 0);
    printf("%s\n", data.buf);

    enum op opNum;
    int argc = 0;
    char argv_tmp[4096] = {0};
    sscanf(data.buf, "{op=%d,argc=%d}{%s}", &opNum, &argc, argv_tmp);

    char* argv[4];
    memset(argv, 0, sizeof(argv));
    argv[0] = strtok(argv_tmp, ",");
    for(int i = 1; i < argc; i++){
        argv[i] = strtok(NULL, ",");
    }

    char cwd[1024] = {0};
    strcat(cwd, root);
    switch(opNum){
        case LOGIN:
            login(pool, net_fd, argc, argv);
            break;
        case CD:
            cd(pool, net_fd, argc, argv);
            break;
        case LS:
            ls();
            break;
        case PWD:

    }
}

int ls(int net_fd, char* dirpath){
    DIR* pdir = opendir(dirpath);

    struct dirent* pDirent = readdir(pdir);
    data_t data;
    while(pDirent != NULL){
        strcat(data.buf, " ");
        strcat(data.buf, pDirent->d_name);
        pDirent = readdir(pdir);
    }
    data.size = strlen(data.buf);
    send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
    return 0;
}

int pwd(int net_fd, char* cwd){

}