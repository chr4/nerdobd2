/* this file was heavily "inspired" (read: copied) from cgps.c
 * thank you, gpsd team!
 */
#include "gps.h"

static struct gps_data_t gpsdata;

int
get_gps_data(struct gps_fix_t *g)
{
    if (gps_read(&gpsdata) <= 0)
    {   
        puts("gps_read() error");
        return -1;
    }

    memcpy(g, &gpsdata.fix, sizeof(struct gps_fix_t));

    // convert speed to km/h
    g->speed = g->speed * 3.6;
    g->eps = g->eps * 3.6;

    return 0; 
}

void
gps_stop(void)
{
    // disconnect from gpsd
    printf("disconnecting from gpsd...\n");
    gps_close( (struct gps_data_t *) &gps_data);
}

int
gps_start(void)
{
    // open the stream to gpsd
    if (gps_open(GPSD_SHARED_MEMORY, NULL, &gpsdata) != 0) {
        printf("couldn't connect to gpsd: %s\n", gps_errstr(errno));
        return -1;
    }

    return 0;
}

