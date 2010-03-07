#include "serial.h"


/* array with strings for rrdtool global options */
char *rrdstyle[256] =
{ 
    "--slope-mode", "--height", "230", "--width", "490", "--full-size-mode", "--rigid",
    "--x-grid", "SECOND:30:MINUTE:1:MINUTE:1:0:%R",
    "--color", "SHADEB#222222", "--color", "SHADEA#222222",
    "--color", "BACK#222222", "--color", "FRAME#222222",
    "--color", "GRID#aaaaaa00", "--color", "MGRID#aaaaaa00",
    "--color", "CANVAS#eeeeee00", "--color", "AXIS#aaaaaa",
    "--color", "FONT#aaaaaa", "--color", "ARROW#aaaaaa",
    NULL    // array needs to be terminated with NULL
};


void
rrdtool_update_consumption (void)
{
    char    starttime[256];
    char    endtime[256];    
    time_t  t;
    int     i;
    int     fd;

    float   tmp_short = 0;
    float   tmp_medium = 0;    
    float   tmp_long = 0;
    
    if (con_km < 0)
    {
        snprintf (starttime, sizeof (starttime), "%d::%.2f", 
                  (int) time (&t), con_h);
    }
    else
    {

        // read values from file (in case its been resetted)
        if ( (fd = open( CON_AV_FILE, O_RDONLY )) != -1)
        {
            read(fd, &av_con, sizeof(av_con));
            close( fd );
        }
        
        // save consumption to average consumption array
        if (av_con.counter == LONG)
        {
            av_con.array_full = 1;
            av_con.counter = 0;
        }
        
        av_con.array[av_con.counter++] = con_km;
        

        // calculate average for SHORT seconds
        if (av_con.counter > SHORT)
            for (i = av_con.counter - SHORT; i < av_con.counter; i++)
                tmp_short += av_con.array[i];
        else
            for (i = 0; i < av_con.counter; i++)
                tmp_short += av_con.array[i];
        
        av_con.average_short = tmp_short / i;
        
        
        // calculate average for MEDIUM seconds
        if (av_con.counter > MEDIUM)
            for (i = av_con.counter - MEDIUM; i < av_con.counter; i++)
                tmp_medium += av_con.array[i];
        else
            for (i = 0; i < av_con.counter; i++)
                tmp_medium += av_con.array[i];
        
        av_con.average_medium = tmp_medium / i;

        
        // calculate average for LONG seconds
        if (av_con.array_full)
            for (i = 0; i < LONG; i++)
                tmp_long += av_con.array[i];
        else
            for (i = 0; i < av_con.counter; i++)
                tmp_long += av_con.array[i];
        
        av_con.average_long = tmp_long / i;


        // save consumption array to file        
        if ( (fd = open( CON_AV_FILE, O_WRONLY|O_CREAT, 00644 )) == -1)
            perror("couldn't open file:\n");
        else
        {
            write(fd, &av_con, sizeof(av_con));
            close(fd);
        }
        
        
        snprintf (starttime, sizeof (starttime), "%d:%.2f:%.2f", 
                  (int) time (&t), con_km, con_h);
    }
    
    
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
            "AREA:con_km#f00000ee:l/100km", 
            "LINE2:con_h#aaaaaaee:l/h",
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
    int     i;
    int     fd;

    float   tmp_short = 0;
    float   tmp_medium = 0;    
    float   tmp_long = 0;
    
    if (speed >= 0)
    { 
        // read values from file (in case its been resetted)
        if ( (fd = open( SPEED_AV_FILE, O_RDONLY )) != -1)
        {
            read(fd, &av_speed, sizeof(av_speed));
            close( fd );
        }


        // save speed to average speed array
        if (av_speed.counter == LONG)
        {
            av_speed.array_full = 1;
            av_speed.counter = 0;
        }

        av_speed.array[av_speed.counter++] = speed;


        // calculate average for SHORT seconds
        if (av_speed.counter > SHORT)
            for (i = av_speed.counter - SHORT; i < av_speed.counter; i++)
                tmp_short += av_speed.array[i];
        else
            for (i = 0; i < av_speed.counter; i++)
                tmp_short += av_speed.array[i];

        av_speed.average_short = tmp_short / i;


        // calculate average for MEDIUM seconds
        if (av_speed.counter > MEDIUM)
            for (i = av_speed.counter - MEDIUM; i < av_speed.counter; i++)
                tmp_medium += av_speed.array[i];
        else
            for (i = 0; i < av_speed.counter; i++)
                tmp_medium += av_speed.array[i];

        av_speed.average_medium = tmp_medium / i;


        // calculate average for LONG seconds
        if (av_speed.array_full)
            for (i = 0; i < LONG; i++)
                tmp_long += av_speed.array[i];
        else
            for (i = 0; i < av_speed.counter; i++)
                tmp_long += av_speed.array[i];

        av_speed.average_long = tmp_long / i;


        // save av_speed array to file    
        if ( (fd = open( SPEED_AV_FILE, O_WRONLY|O_CREAT, 00644 )) == -1)
            perror("couldn't open file:\n");
        else
        {
            write(fd, &av_speed, sizeof(av_speed));
            close(fd);
        }
        
        snprintf (starttime, sizeof (starttime), "%d:%.1f", 
                  (int) time (&t), speed);
        
        if (fork() == 0)
        {
            execlp("rrdtool", "rrdtool",
                   "update", "speed.rrd", starttime,
                   NULL);
            
            exit(0);
        }
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
            "AREA:myspeed#f00000ee:speed", 
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
                "DS:km:GAUGE:30:U:U", "DS:h:GAUGE:5:0:U", // unknown after 30sec
                "RRA:AVERAGE:0.5:1:300", 
                "RRA:AVERAGE:0.5:300:1",   // 5min
                "RRA:AVERAGE:0.5:5:360", "RRA:AVERAGE:0.5:1800:1",  // 30min
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
                "DS:speed:GAUGE:30:0:200",   // unknown after 30sec
                "RRA:AVERAGE:0.5:1:300", 
                "RRA:AVERAGE:0.5:300:1",   // 5min 
                "RRA:AVERAGE:0.5:5:360", "RRA:AVERAGE:0.5:1800:1",  // 30min
                NULL);
        exit (0);
    }
    
    waitpid (pid, &status, 0);
    
    return;
}
