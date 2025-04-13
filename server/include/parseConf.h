#ifndef __PARSE_CONF_H__
#define __PARSE_CONF_H__

#include "heads.h"

int parse_conf(char *conf_path, struct sockaddr_in* cmd_addr, struct sockaddr_in* data_addr);

#endif