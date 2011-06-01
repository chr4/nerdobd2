#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

// for file operations
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "config.h"

#include "json.h"
#include "sqlite.h"

int httpd_start(sqlite3 *);

const char *json_latest_data(sqlite3 *);
const char *json_graph_data(sqlite3 *, char *, unsigned long int, unsigned long int);
