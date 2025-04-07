#include "tcpLink.h"

int tcpInit(int* sock_fd, struct sockaddr_in* addr, int n){
    ERR_CHECK((*sock_fd = socket(AF_INET, SOCK_STREAM, 0)), -1, "socket))");

    int reuse = 1;
    setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ERR_CHECK(bind(*sock_fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in)), -1, "bind");
    
    ERR_CHECK(listen(*sock_fd, n), -1, "listen");
}