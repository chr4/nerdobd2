#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <gps.h>

#include "common.h"
#include "config.h"

#include "sqlite.h"

int gps_start(sqlite3 *);
