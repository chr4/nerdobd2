#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

// for file operations
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../common/common.h"
#include "../common/config.h"

#include "../common/json.h"

int open_db(void);

json_object *json_averages(int);
json_object *json_latest_data(void);
json_object *json_generate_graph(char *, int);

const char *json_generate(long, long, long);
