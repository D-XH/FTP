#include "mysqlConn.h"

MYSQL* connect_mysql()
{
    MYSQL* mysql = mysql_init(NULL);
    if(mysql == NULL){
        printf("%s\n", mysql_error(mysql));
    }
    mysql = mysql_real_connect(mysql, "localhost", "deng", "deng", "cloudDisk", 0, NULL, 0);
    if(mysql == NULL){
        printf("%s\n", mysql_error(mysql));
    }
    return mysql;
}

int disconnect_mysql(MYSQL *mysql, MYSQL_RES* query_res)
{
    if(query_res != NULL){
        mysql_free_result(query_res);
    }
    mysql_close(mysql);
    return 0;
}
