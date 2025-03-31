#ifndef __HEADS_H__
#define __HEADS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>

#include "log.h"

// #define ERR_CHECK(key, val, err) {if(key == val){log_err(err);return -1;}}
#define ERR_CHECK(key, val, err) {if(key == val){perror(err);return -1;}}
#endif