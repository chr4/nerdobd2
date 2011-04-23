#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

// for copying file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// sqlite database
#include <sqlite3.h>

// tcp sockets
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// include common defines
#include "../common/common.h"
#include "../common/config.h"

// sqlite functions
int   exec_query(char *);
int   init_db(void);
void  close_db(void);
void  sync_db(void);
