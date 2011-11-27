/* this file was heavily "inspired" (read: copied) from cgps.c
 * thank you, gpsd team!
 */
#include "gps.h"

#ifdef GPS
sqlite3 *db;

static struct gps_data_t gpsdata;

double last_latitude = -1;
double last_longitude = -1;

void
insert_gps_data(struct gps_fix_t g)
{
    char query[LEN_QUERY];
    double rounded_latitude, rounded_longitude;

    // return if we don't have a gps fix
    if (g.mode < MODE_2D)
        return;

    // if a value is not set, replace with -1
    if (isnan(g.latitude) != 0)
      g.latitude = -1;
    if (isnan(g.longitude) != 0)
      g.longitude = -1;
    if (isnan(g.altitude) != 0)
      g.altitude = -1;

    if (isnan(g.speed) != 0)
      g.speed = -1;
    if (isnan(g.climb) != 0)
      g.climb = -1;
    if (isnan(g.track) != 0)
      g.track = -1;

    if (isnan(g.epy) != 0)
      g.epy = -1;
    if (isnan(g.epx) != 0)
      g.epx = -1;
    if (isnan(g.epv) != 0)
      g.epv = -1;
    if (isnan(g.eps) != 0)
      g.eps = -1;
    if (isnan(g.epc) != 0)
      g.epc = -1;
    if (isnan(g.epd) != 0)
      g.epd = -1;

    // return if lat/long wasn't changed (more than xx.xxxxx)
    rounded_latitude  = floorf(g.latitude * 100000) / 100000;
    rounded_longitude = floorf(g.longitude * 100000) / 100000;

    if (rounded_latitude == last_latitude && rounded_longitude == last_longitude)
      return;

    last_latitude  = rounded_latitude;
    last_longitude = rounded_longitude;


    exec_query(db, "BEGIN TRANSACTION");

    snprintf(query, sizeof(query),
             "INSERT INTO gps_data VALUES ( \
             NULL, DATETIME('NOW', 'localtime'), \
             %d, %f, %f, %f, \
             %f, %f, %f, \
             %f, %f, %f, %f, %f, %f )",
             g.mode, g.latitude, g.longitude, g.altitude,
             g.speed * 3.6, g.climb, g.track, // * 3.6 to convert to km/h
             g.epy, g.epx, g.epv, g.eps * 3.6, g.epc, g.epd);

    exec_query(db, query);

    exec_query(db, "END TRANSACTION");
}


/* This gets called once for each new GPS sentence. */
static void
update_gps(struct gps_data_t *gpsdata,
           char *message, size_t len UNUSED)
{
   insert_gps_data(gpsdata->fix);
}

void
gps_stop(int signo)
{
    // TODO: cleanly deregister from gpsd

    printf(" - child (gps): closing database...\n");
    close_db(db);

    _exit(0);
}

int
gps_start(sqlite3 *mydb)
{
    db = mydb;

    struct timeval timeout;
    fd_set rfds;
    int data;
    pid_t pid;

    // open the stream to gpsd
    if (gps_open_r(NULL, NULL, &gpsdata) != 0) {
        printf("couldn't connect to gpsd: %s\n", gps_errstr(errno));
	return -1;
    }

    if ( (pid = fork()) == 0)
    {  
        // add signal handler for cleanup function
        signal(SIGINT, gps_stop);
        signal(SIGTERM, gps_stop);

        // run update_gps everytime new data is available
        gps_set_raw_hook(&gpsdata, update_gps);

        // register at gpsd
        gps_stream(&gpsdata, WATCH_ENABLE, NULL);

        for (;;) {

            // watch to see when it has input
            FD_ZERO(&rfds);
            FD_SET(gpsdata.gps_fd, &rfds);

            // wait up to five seconds.
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            // check if we have new information
            data = select(gpsdata.gps_fd + 1, &rfds, NULL, NULL, &timeout);

            if (data == -1) {
                puts("gps: select() error");
                return -1;
            } else if (data) {
                    errno = 0;
                if (gps_read(&gpsdata) == -1) {
                    puts("gps_read() error");
                    return -1;
                }
            }
        }

        // should never be reached
        _exit(0);
    }

    return pid;
}
#endif
