/* this file was heavily "inspired" (read: copied) from cgps.c
 * thank you, gpsd team!
 */
#include "gps.h"

static struct gps_data_t gpsdata;
char    gps_available = 0;


int
get_gps_data(struct gps_fix_t *g) {

    if (!gps_available)
        return -1;

    if (!gps_waiting(&gpsdata, 10)) {
        fprintf(stderr, "no gps data available (timeout)\n");
        return -1;
    }

    errno = 0;
    if (gps_read(&gpsdata) == -1) {
        perror("gps_read()");
        return -1;
    }

    memcpy(g, &gpsdata.fix, sizeof(struct gps_fix_t));

    // convert speed to km/h
    g->speed = g->speed * 3.6;
    g->eps = g->eps * 3.6;

    return 0;
}

void
gps_stop(void) {

    if (!gps_available)
        return;

    gps_stream((struct gps_data_t *) &gpsdata, WATCH_DISABLE, NULL);
    gps_close((struct gps_data_t *) &gpsdata);
}


int
gps_start(void) {

    /* open the stream to gpsd
     * shared memory segment kinda isn't available anymore?
     *
     * if (gps_open(GPSD_SHARED_MEMORY, NULL, &gpsdata) != 0) {
     */
    if (gps_open("localhost", DEFAULT_GPSD_PORT, &gpsdata) != 0) {
        printf("couldn't connect to gpsd: %s\n", gps_errstr(errno));
        return -1;
    }

    // watching is not available (and not needed) when using shared memory segment
    if (gps_stream((struct gps_data_t *) &gpsdata, WATCH_ENABLE, NULL) == -1)
        perror("gps_stream()");

    gps_available = 1;
    return 0;
}
