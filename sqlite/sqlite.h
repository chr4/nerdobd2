#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

// sqlite database
#include <sqlite3.h>

// unix domain sockets
#include <sys/socket.h>
#include <sys/un.h>


// sqlite functions
int     exec_query(char *);
int     create_table(char *);
int     insert_value(char *, float);
float   get_value(char *);
float   get_row(char *, char *);
float   get_average(char*, char *, int);
int     calc_consumption(void);
int     init_db(void);
void    close_db(void);

