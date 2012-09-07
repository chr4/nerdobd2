#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

// for file operations
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "config.h"

#include "json.h"

#ifdef DB_SQLITE
#   include "sqlite.h"
#   define DB_ADAPTER sqlite3
#endif

#ifdef DB_POSTGRES
#   include "postgres.h"
#   define DB_ADAPTER PGconn
#endif

const char *json_get_data(DB_ADAPTER *);
const char *json_get_averages(DB_ADAPTER *);
const char *json_graph_data(DB_ADAPTER *, char *, unsigned long int,
                            unsigned long int);

int     httpd_start(void);
