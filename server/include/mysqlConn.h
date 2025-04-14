#ifndef __MYSQL_CONN__
#define __MYSQL_CONN__

#include "heads.h"
#include <mysql/mysql.h>

extern char MYSQL_HOST[];
extern char MYSQL_USERNAME[];
extern char MYSQL_PASSWD[];
extern char DATABASE_NAME[];

MYSQL* connect_mysql();
int disconnect_mysql(MYSQL* mysql, MYSQL_RES* query_res);

#endif