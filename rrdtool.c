#include "serial.h"


/* array with strings for rrdtool global options */
char *rrdstyle[256] =
{ 
    "--slope-mode", "--height", "230", "--width", "490", "--full-size-mode", "--rigid",
    "--x-grid", "SECOND:30:MINUTE:1:MINUTE:1:0:%R",
    "--color", "SHADEB#222222", "--color", "SHADEA#222222",
    "--color", "BACK#222222", "--color", "FRAME#222222",
    "--color", "GRID#aaaaaa", "--color", "MGRID#aaaaaa",
    "--color", "CANVAS#eeeeee", "--color", "AXIS#aaaaaa",
    "--color", "FONT#aaaaaa", "--color", "ARROW#aaaaaa",
    NULL    // array needs to be terminated with NULL
};


void
rrdtool_update_consumption (void)
{
    char    starttime[256];
    char    endtime[256];    
    time_t  t;
    
    if (con_km < 0)
        snprintf (starttime, sizeof (starttime), "%d::%.2f", 
                  (int) time (&t), con_h);
    else
        snprintf (starttime, sizeof (starttime), "%d:%.2f:%.2f", 
                  (int) time (&t), con_km, con_h);
    
    if (fork() == 0)
    {
        execlp("rrdtool", "rrdtool",
               "update", "consumption.rrd", starttime,
               NULL);

        exit(0);
    }
    
    // we want to graph the last 5 mins
    snprintf(starttime, sizeof(starttime), "%d",
             (int) time (&t) - 300); // now - 300s (5min)
    
    snprintf(endtime, sizeof(endtime), "%d",
             (int) time (&t));
    
    
    if (fork() == 0)
    {
        int i = 0, n = 0;
        char combined[256][256];
        
        char *args[256] =
        {             
            "rrdtool", "graph", "consumption.png",
            "--start", starttime, "--end", endtime,
            "--upper-limit", "19.5", "--lower-limit", "0", "--y-grid", "3.25:1",
            "DEF:con_km=consumption.rrd:km:AVERAGE", 
            "DEF:con_h=consumption.rrd:h:AVERAGE", 
            "AREA:con_km#f00000:l/100km", 
            "LINE2:con_h#00f000:l/h",
            NULL    // array needs to be terminated with NULL
        };
        
        
        
        // append args to combined
        while (args[i] != NULL) 
        {
            strncpy(combined[i], args[i], sizeof(combined[i]));
            i++;
        }
        
        // append rrdstyle to combined
        while (rrdstyle[n] != NULL) 
        {
            strncpy(combined[i], rrdstyle[n], sizeof(combined[i]));
            n++;
            i++;
        }
        
        // we need an array of pointers for execvp()
        for (n = 0; n < i; n++)
            args[n] = combined[n];
        
        
        execvp(args[0], args);
        exit(0);
    }
    
    return;
}

void
rrdtool_update_speed (void)
{
    char    starttime[256];
    char    endtime[256];
    time_t  t;
    
    snprintf (starttime, sizeof (starttime), "%d:%.1f", 
              (int) time (&t), speed);
    
    if (fork() == 0)
    {
        execlp("rrdtool", "rrdtool",
               "update", "speed.rrd", starttime,
               NULL);
        
        exit(0);
    }

    // we want to graph the last 5 mins
    snprintf(starttime, sizeof(starttime), "%d",
             (int) time (&t) - 300); // now - 300s (5min)
    
    snprintf(endtime, sizeof(endtime), "%d",
             (int) time (&t));
    
    
    if (fork() == 0)
    {
        int i = 0, n = 0;
        char combined[256][256];

        char *args[256] =
        { 
            "rrdtool", "graph", "speed.png",
            "--start", starttime, "--end", endtime,
            "--upper-limit", "135", "--lower-limit", "0", "--y-grid", "7.5:2",
            "DEF:myspeed=speed.rrd:speed:AVERAGE",
            "AREA:myspeed#0000f0:speed", 
            NULL    // array needs to be terminated with NULL
        };
        
        
        // append args to combined
        while (args[i] != NULL) 
        {
            strncpy(combined[i], args[i], sizeof(combined[i]));
            i++;
        }
        
        // append rrdstyle to combined
        while (rrdstyle[n] != NULL) 
        {
            strncpy(combined[i], rrdstyle[n], sizeof(combined[i]));
            n++;
            i++;
        }
        
        // we need an array of pointers for execvp()
        for (n = 0; n < i; n++)
            args[n] = combined[n];

        
        execvp(args[0], args);
        exit(0);
    }
    
    return;
}

void
rrdtool_create_consumption (void)
{
    pid_t   pid;
    int     status;
    time_t  t;
    FILE   *fp;
    
    fp = fopen ("consumption.rrd", "rw");
    if (fp)
    {
        fclose (fp);
        return;
    }
    
    printf ("creating consumption rrd database\n");
    
    /*
     // remove old file
     if (unlink(cmd) == -1)
     perror("could not delete old .rrd file");
     */
    
    if ((pid = fork ()) == 0)
    {
        char    starttime[256];
        
        snprintf (starttime, sizeof (starttime), "%d", (int) time (&t));
        
        // after 15secs: unknown value
        // 1. RRA last 5 mins
        // 2. RRA last 30mins
        // 3. RRA one value (average consumption last 30mins)
        execlp ("rrdtool", "rrdtool", "create", "consumption.rrd",
                "--start", starttime, "--step", "1",
                "DS:km:GAUGE:5:U:U", "DS:h:GAUGE:5:0:U", // unknown after 5sec
                "RRA:AVERAGE:0.5:1:300", 
                /* 
                "RRA:AVERAGE:0.5:300:1",   // 5min
                "RRA:AVERAGE:0.5:5:360", "RRA:AVERAGE:0.5:1800:1",  // 30min
                 */
                NULL);
        exit (0);
    }
    
    waitpid (pid, &status, 0);
    
    return;
}

void
rrdtool_create_speed (void)
{
    pid_t   pid;
    int     status;
    time_t  t;
    FILE   *fp;
    
    fp = fopen ("speed.rrd", "rw");
    if (fp)
    {
        fclose (fp);
        return;
    }
    
    printf ("creating speed rrd database\n");
    
    /*
     // remove old file
     if (unlink(cmd) == -1)
     perror("could not delete old .rrd file");
     */
    
    if ((pid = fork ()) == 0)
    {
        char    starttime[256];
        
        snprintf (starttime, sizeof (starttime), "%d", (int) time (&t));
        
        // rrdtool create
        execlp ("rrdtool", "rrdtool", "create", "speed.rrd",
                "--start", starttime, "--step", "1",
                "DS:speed:GAUGE:5:0:200",   // unknown after 5sec
                "RRA:AVERAGE:0.5:1:300", 
                /*
                "RRA:AVERAGE:0.5:300:1",   // 5min 
                "RRA:AVERAGE:0.5:5:360", "RRA:AVERAGE:0.5:1800:1",  // 30min
                 */
                NULL);
        exit (0);
    }
    
    waitpid (pid, &status, 0);
    
    return;
}