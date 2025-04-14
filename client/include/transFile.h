#ifndef __TRANS_FILE__
#define __TRANS_FILE__

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

typedef struct data_s{
    size_t size;
    char buf[1024];
}data_t;

size_t recvn(int net_fd, void* data, size_t n);
int recvFile(int net_fd, char* file_path);
int trans(int net_fd, char *file_path);
int trans_mmap(int net_fd, char* file_path);

#endif