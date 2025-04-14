#ifndef __PARSE_CONF_H__
#define __PARSE_CONF_H__

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
int parse_conf(char* conf_path, struct sockaddr_in* cmd_addr, struct sockaddr_in* data_addr);

#endif