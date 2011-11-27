/* this file was heavily "inspired" (read: copied) from cgps.c
 * thank you, gpsd team!
 */
#include "gps.h"

#ifdef GPS
sqlite3 *db;

static struct gps_data_t gpsdata;
static time_t status_timer;     /* Time of last state change. */
static bool magnetic_flag = false;

/* Convert true heading to magnetic.  Taken from the Aviation
   Formulary v1.43.  Valid to within two degrees within the
   continiental USA except for the following airports: MO49 MO86 MO50
   3K6 02K and KOOA.  AK correct to better than one degree.  Western
   Europe correct to within 0.2 deg.

   If you're not in one of these areas, I apologize, I don't have the
   math to compute your varation.  This is obviously extremely
   floating-point heavy, so embedded people, beware of using.

   Note that there are issues with using magnetic heading.  This code
   does not account for the possibility of travelling into or out of
   an area of valid calculation beyond forcing the magnetic conversion
   off.  A better way to communicate this to the user is probably
   desirable (in case the don't notice the subtle change from "(mag)"
   to "(true)" on their display).
 */
static float true2magnetic(double lat, double lon, double heading)
{
    // Western Europe
    if ((lat > 36.0) && (lat < 68.0) && (lon > -10.0) && (lon < 28.0)) {
	heading =
	    (10.4768771667158 - (0.507385322418858 * lon) +
	     (0.00753170031703826 * pow(lon, 2))
	     - (1.40596203924748e-05 * pow(lon, 3)) -
	     (0.535560699962353 * lat)
	     + (0.0154348808069955 * lat * lon) -
	     (8.07756425110592e-05 * lat * pow(lon, 2))
	     + (0.00976887198864442 * pow(lat, 2)) -
	     (0.000259163929798334 * lon * pow(lat, 2))
	     - (3.69056939266123e-05 * pow(lat, 3)) + heading);
    }
    // USA 
    else if ((lat > 24.0) && (lat < 50.0) && (lon > 66.0) && (lon < 125.0)) {
	lon = 0.0 - lon;
	heading =
	    ((-65.6811) + (0.99 * lat) + (0.0128899 * pow(lat, 2)) -
	     (0.0000905928 * pow(lat, 3)) + (2.87622 * lon)
	     - (0.0116268 * lat * lon) - (0.00000603925 * lon * pow(lat, 2)) -
	     (0.0389806 * pow(lon, 2))
	     - (0.0000403488 * lat * pow(lon, 2)) +
	     (0.000168556 * pow(lon, 3)) + heading);
    }
    // AK 
    else if ((lat > 54.0) && (lon > 130.0) && (lon < 172.0)) {
	lon = 0.0 - lon;
	heading =
	    (618.854 + (2.76049 * lat) - (0.556206 * pow(lat, 2)) +
	     (0.00251582 * pow(lat, 3)) - (12.7974 * lon)
	     + (0.408161 * lat * lon) + (0.000434097 * lon * pow(lat, 2)) -
	     (0.00602173 * pow(lon, 2))
	     - (0.00144712 * lat * pow(lon, 2)) +
	     (0.000222521 * pow(lon, 3)) + heading);
    } else {
	// We don't know how to compute magnetic heading for this location
	magnetic_flag = false;
    }

    // No negative headings. 
    if (heading < 0.0)
	heading += 360.0;

    return (heading);
}



/* This gets called once for each new GPS sentence. */
static void
update_gps(struct gps_data_t *gpsdata,
           char *message, size_t len UNUSED)
{
    int i, j;
    char scr[128];
    int newstate;
    bool usedflags[MAXCHANNELS];

    static float altfactor = 1;
    static float speedfactor = 3.6;
    static char *altunits = "m";
    static char *speedunits = "km/h";


    /* must build bit vector of which statellites are used from list */
    for (i = 0; i < MAXCHANNELS; i++) {
	usedflags[i] = false;
	for (j = 0; j < gpsdata->satellites_used; j++)
	    if (gpsdata->used[j] == gpsdata->PRN[i])
		usedflags[i] = true;
    }


    /* Fill in the latitude. */
    if (gpsdata->fix.mode >= MODE_2D && isnan(gpsdata->fix.latitude) == 0)
        printf("%f\n", gpsdata->fix.latitude); 
    else
        puts("n/a");

    /* Fill in the longitude. */
    if (gpsdata->fix.mode >= MODE_2D && isnan(gpsdata->fix.longitude) == 0)
        printf("%f\n", gpsdata->fix.longitude);
    else
        puts("n/a");

    /* Fill in the altitude. */
    if (gpsdata->fix.mode == MODE_3D && isnan(gpsdata->fix.altitude) == 0)
	(void)snprintf(scr, sizeof(scr), "%.1f %s",
		       gpsdata->fix.altitude * altfactor, altunits);
    else
	(void)snprintf(scr, sizeof(scr), "n/a");
    puts(scr);
    
    /* Fill in the speed. */
    if (gpsdata->fix.mode >= MODE_2D && isnan(gpsdata->fix.track) == 0)
	(void)snprintf(scr, sizeof(scr), "%.1f %s",
		       gpsdata->fix.speed * speedfactor, speedunits);
    else
	(void)snprintf(scr, sizeof(scr), "n/a");
    puts(scr);
    
    /* Fill in the heading. */
    if (gpsdata->fix.mode >= MODE_2D && isnan(gpsdata->fix.track) == 0)
    {
	if (!magnetic_flag) {
	    (void)snprintf(scr, sizeof(scr), "%.1f deg (true)",
			   gpsdata->fix.track);
	} else {
	    (void)snprintf(scr, sizeof(scr), "%.1f deg (mag) ",
			   true2magnetic(gpsdata->fix.latitude,
					 gpsdata->fix.longitude,
					 gpsdata->fix.track));
        }
    } else
	(void)snprintf(scr, sizeof(scr), "n/a");
    puts(scr);
    
    /* Fill in the rate of climb. */
    if (gpsdata->fix.mode == MODE_3D && isnan(gpsdata->fix.climb) == 0)
	(void)snprintf(scr, sizeof(scr), "%.1f %s/min",
		       gpsdata->fix.climb * altfactor * 60, altunits);
    else
	(void)snprintf(scr, sizeof(scr), "n/a");
    puts(scr);
    
    /* Fill in the GPS status and the time since the last state
     * change. */
    if (gpsdata->online == 0) {
	newstate = 0;
	(void)snprintf(scr, sizeof(scr), "OFFLINE");
    } else {
	newstate = gpsdata->fix.mode;
	switch (gpsdata->fix.mode) {
	case MODE_2D:
	    (void)snprintf(scr, sizeof(scr), "2D %sFIX (%d secs)",
			   (gpsdata->status ==
			    STATUS_DGPS_FIX) ? "DIFF " : "",
			   (int)(time(NULL) - status_timer));
	    break;
	case MODE_3D:
	    (void)snprintf(scr, sizeof(scr), "3D %sFIX (%d secs)",
			   (gpsdata->status ==
			    STATUS_DGPS_FIX) ? "DIFF " : "",
			   (int)(time(NULL) - status_timer));
	    break;
	default:
	    (void)snprintf(scr, sizeof(scr), "NO FIX (%d secs)",
			   (int)(time(NULL) - status_timer));
	    break;
	}
    }
    puts(scr);
    
	/* Fill in the estimated horizontal position error. */
	if (isnan(gpsdata->fix.epx) == 0)
	    (void)snprintf(scr, sizeof(scr), "+/- %d %s",
			   (int)(gpsdata->fix.epx * altfactor), altunits);
	else
	    (void)snprintf(scr, sizeof(scr), "n/a");
       puts(scr);

	if (isnan(gpsdata->fix.epy) == 0)
	    (void)snprintf(scr, sizeof(scr), "+/- %d %s",
			   (int)(gpsdata->fix.epy * altfactor), altunits);
	else
	    (void)snprintf(scr, sizeof(scr), "n/a");
    puts(scr);
    
	/* Fill in the estimated vertical position error. */
	if (isnan(gpsdata->fix.epv) == 0)
	    (void)snprintf(scr, sizeof(scr), "+/- %d %s",
			   (int)(gpsdata->fix.epv * altfactor), altunits);
	else
	    (void)snprintf(scr, sizeof(scr), "n/a");
    puts(scr);
    
	/* Fill in the estimated track error. */
	if (isnan(gpsdata->fix.epd) == 0)
	    (void)snprintf(scr, sizeof(scr), "+/- %d deg",
			   (int)(gpsdata->fix.epd));
	else
	    (void)snprintf(scr, sizeof(scr), "n/a");
    puts(scr);
    
	/* Fill in the estimated speed error. */
	if (isnan(gpsdata->fix.eps) == 0)
	    (void)snprintf(scr, sizeof(scr), "+/- %d %s",
			   (int)(gpsdata->fix.eps * speedfactor), speedunits);
	else
	    (void)snprintf(scr, sizeof(scr), "n/a");
        puts(scr);
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

    /* Open the stream to gpsd. */
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

        status_timer = time(NULL);

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
