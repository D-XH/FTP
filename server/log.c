#include "log.h"

int log_init()
{
    openlog("CD_LOG", LOG_PID|LOG_CONS, LOG_USER);
    return 0;
}
int log_close(){
    closelog();
}
void log_debug(char *debug)
{
    syslog(LOG_DEBUG, "file: %s, line:%d, %s\n", __FILE__, __LINE__, debug);
}

void log_err(char* err){
    syslog(LOG_ERR, "file: %s, line:%d, %s\n", __FILE__, __LINE__, err);
}

void log_info(char* info){
    syslog(LOG_INFO, "file: %s, line:%d, %s\n", __FILE__, __LINE__, info);
}
