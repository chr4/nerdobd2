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

// json parser
#include <json/json.h>

// include common defines
#include "../common/common.h"

// sqlite functions
int   exec_query(char *);
int   create_table(char *);
float get_value(char *);
float get_row(char *, char *);
float get_average(char*, char *, int);
int   init_db(void);
void  close_db(void);
void  sync_db(void);
const char *json_generate(int, int, int);

// json helper functions
json_object *add_string(json_object *, char *, char *);
json_object *add_int(json_object *, char *, int);
json_object *add_double(json_object *, char *, double);
json_object *add_boolean(json_object *, char *, char);
json_object *add_array(json_object *, char *);
json_object *add_object(json_object *, char *);
json_object *add_data(json_object *, double, double);

// the ajax html server
void  ajax_socket (int port);
