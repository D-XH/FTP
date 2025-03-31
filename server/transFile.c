#include "transFile.h"

ssize_t recvn(int net_fd, void* data, ssize_t n){
    char* p = (char*)data;
    ssize_t cnt = 0;
    while(cnt < n){
        ssize_t rsize = recv(net_fd, p, n, 0);
        if(rsize == 0){
            return -1;
        }
        cnt += rsize;
    }
    return cnt;
}

int recvFile(int net_fd, char* file_path){
    printf("file: %s\n", file_path);
    umask(0000);
    int file_fd = open(file_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(file_fd == -1){
        perror("open file");
        return -1;
    }

    data_t data;
    while(1){
        memset(&data, 0, sizeof(data));
        recvn(net_fd, &data.size, sizeof(data.size));
        recvn(net_fd, data.buf, data.size);
        if(data.size == 0){
            break;
        }
        write(file_fd, data.buf, data.size);
    }
    close(file_fd);
    return 0;
}

int trans(int net_fd, char *file_path){
    int file_fd = open(file_path, O_RDONLY);
    if(file_fd == -1){
        perror("open file");
        return -1;
    }

    data_t data;
    // memset(&data, 0, sizeof(data));

    // // send filename
    // char* file_name = strrchr(file_path, '/') + 1;
    // data.size = strlen(file_name);
    // memcpy(data.buf, file_name, data.size);
    // send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);

    while(1){
        memset(&data, 0, sizeof(data));
        ssize_t rsize = read(file_fd, data.buf, sizeof(data.buf));
        data.size = rsize;
        send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);
        if(rsize == 0){
            break;
        }
    }
    close(file_fd);
    return 0;
}

int trans_mmap(int net_fd, char *file_path)
{
    int file_fd = open(file_path, O_RDWR);
    if(file_fd == -1){
        perror("open file");
        return -1;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    data_t data;
    memset(&data, 0, sizeof(data));
    char* file_name = strrchr(file_path, '/') + 1;
    data.size = strlen(file_name);
    memcpy(data.buf, file_name, data.size);
    send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);

    data.size = sizeof(file_size);
    memcpy(data.buf, &file_size, data.size);
    send(net_fd, &data, sizeof(data.size)+data.size, MSG_NOSIGNAL);

    char* p = (char*)mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, 0);
    if(p == MAP_FAILED){
        perror("mmap");
        return -1;
    }
    send(net_fd, p, file_size, MSG_NOSIGNAL);

    munmap(p, file_size);
    close(file_fd);
    return 0;
}