#include "serial.h"

void *
rrdtool_update_consumption ()
{
    char    cmd[256];
    time_t  t;
    
    if (con_km < 0)
        snprintf (cmd, sizeof (cmd), "rrdtool update consumption.rrd %d::%.2f &", 
                  (int) time (&t), con_h);
    else
        snprintf (cmd, sizeof (cmd), "rrdtool update consumption.rrd %d:%.2f:%.2f &", 
                  (int) time (&t), con_km, con_h);
    
    if (system(cmd) == -1)
        perror("system() ");
        
    
    
    snprintf (cmd, sizeof (cmd), 
              "rrdtool graph consumption.png --start %d --end %d DEF:con_km=consumption.rrd:km:AVERAGE AREA:con_km#990000:l/100km DEF:con_h=consumption.rrd:h:AVERAGE LINE3:con_h#009999:l/h &> /dev/null &", 
              (int) time (&t) - 300, (int) time (&t) );
    
    if (system(cmd) == -1)
        perror("system() "); 
    
    return (void *) NULL;
}

void *
rrdtool_update_speed ()
{
    char    cmd[256];
    time_t  t;
    
    snprintf (cmd, sizeof (cmd), "rrdtool update speed.rrd %d:%.1f &", 
              (int) time (&t), speed);
    
    if (system(cmd) == -1)
        perror("system() ");    
    snprintf (cmd, sizeof (cmd), 
              "rrdtool graph speed.png --start %d --end %d DEF:myspeed=speed.rrd:speed:AVERAGE LINE2:myspeed#0000FF:speed &> /dev/null &",
              (int) time (&t) - 300, (int) time (&t) );
    
    if (system(cmd) == -1)
        perror("system() ");
    
    return (void *) NULL;
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
                "DS:km:GAUGE:15:U:U", "DS:h:GAUGE:15:0:U",
                "RRA:AVERAGE:0.5:1:300", "RRA:AVERAGE:0.5:5:360",
                "RRA:AVERAGE:0.5:1800:1", NULL);
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
                "DS:speed:GAUGE:15:0:200", 
                "RRA:AVERAGE:0.5:1:300", "RRA:AVERAGE:0.5:5:360",
                "RRA:AVERAGE:0.5:50:288", "RRA:AVERAGE:0.5:1800:1", NULL);
        exit (0);
    }
    
    waitpid (pid, &status, 0);
    
    return;
}