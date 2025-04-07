#ifndef __LOG_H__
#define __LOG_H__

#include <syslog.h>

int log_init();
int log_close();
void log_debug(char* debug);
void log_err(char* err);
void log_info(char* info);

#endif