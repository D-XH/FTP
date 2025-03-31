#include "transFile.h"

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