#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <gps.h>

#include "common.h"
#include "config.h"

#include "sqlite.h"

static struct gps_data_t gpsdata;
static time_t status_timer;     /* Time of last state change. */

static float altfactor = 1;
static float speedfactor = 3.6;
static char *altunits = "m";
static char *speedunits = "km/h";

static bool magnetic_flag = false;

int gps_start(sqlite3 *);
