#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <gps.h>

#include "common.h"
#include "config.h"

int     gps_start(void);
void    gps_stop(void);
int     get_gps_data(struct gps_fix_t *);
