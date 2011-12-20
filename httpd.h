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
#include "sqlite.h"

const char *json_latest_data(sqlite3 *);
const char *json_graph_data(sqlite3 *, char *, unsigned long int, unsigned long int);
#endif


#ifdef DB_POSTGRES
#include "postgres.h"

const char *json_latest_data(PGconn *);
const char *json_graph_data(PGconn *, char *, unsigned long int, unsigned long int);
#endif

int httpd_start(void);
