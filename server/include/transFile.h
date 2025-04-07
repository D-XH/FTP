#ifndef __TRANS_FILE__
#define __TRANS_FILE__

#include "heads.h"

int recvFile(int net_fd, char* file_path);
int trans(int net_fd, char *file_path);
int trans_mmap(int net_fd, char* file_path);

#endif