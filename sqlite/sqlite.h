#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

// for copying file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// sqlite database
#include <sqlite3.h>

// unix domain sockets
#include <sys/socket.h>
#include <sys/un.h>


// sqlite functions
int     exec_query(char *);
int     create_table(char *);
float   get_value(char *);
float   get_row(char *, char *);
float   get_average(char*, char *, int);
int     init_db(void);
void    close_db(void);
void    sync_db(void);

