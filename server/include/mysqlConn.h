#ifndef __MYSQL_CONN__
#define __MYSQL_CONN__

#include "heads.h"
#include <mysql/mysql.h>

MYSQL* connect_mysql();
int disconnect_mysql(MYSQL* mysql, MYSQL_RES* query_res);

#endif