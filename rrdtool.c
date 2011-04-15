#include "serial.h"


/* array with strings for rrdtool global options */
char *rrdstyle[256] =
{ 
    "--slope-mode", "--height", "230", "--width", "490", "--full-size-mode", "--rigid",
    "--color", "SHADEB#222222", "--color", "SHADEA#222222",
    "--color", "BACK#222222", "--color", "FRAME#222222",
    "--color", "GRID#aaaaaa00", "--color", "MGRID#aaaaaa00",
    "--color", "CANVAS#eeeeee00", "--color", "AXIS#aaaaaa",
    "--color", "FONT#aaaaaa", "--color", "ARROW#aaaaaa",
    NULL    // array needs to be terminated with NULL
};


// define x-grid lines for 5min / 30min / 4h
char xgrid_short[256]  = "SECOND:30:MINUTE:1:MINUTE:1:0:%R";
char xgrid_medium[256] = "MINUTE:1:MINUTE:5:MINUTE:5:0:%R";
char xgrid_long[256]   = "MINUTE:15:MINUTE:30:MINUTE:30:0:%R";


void
rrdtool_update_consumption (void)
{
    char    starttime[256];
    char    endtime[256];    
    time_t  t;
    int     i, n;

    char combined[256][256];
    float   per_h;
    float   per_km; 
    
    per_km = get_row("per_km", "consumption"); 
    per_h  = get_row("per_h",  "consumption"); 

    if (per_km < 0)
    {
        snprintf (starttime, sizeof (starttime), "%d::%.2f", 
                  (int) time (&t), per_h);
    }
    else
    {
        snprintf (starttime, sizeof (starttime), "%d:%.2f:%.2f", 
                  (int) time (&t), per_km, per_h);
    }
   
    // rrdtool args
    char *args_update[256] = 
    {
        "update", "consumption.rrd", starttime,
        NULL
    };
    
    for (i = 0; args_update[i] != NULL; i++);

    if (rrd_update(i, args_update) == -1)
        perror("rrd_update() error");

    
    // we want to graph the last 5 mins
    snprintf(starttime, sizeof(starttime), "%d",
             (int) time (&t) - consumption_graph_timespan); // now - timespan
    
    snprintf(endtime, sizeof(endtime), "%d",
             (int) time (&t));
        
    char *args_graph[256] =
    {             
        "graph", CON_GRAPH,
        "--start", starttime, "--end", endtime,
        "--upper-limit", "19.5", "--lower-limit", "0", "--y-grid", "3.25:1",
        "DEF:con_km=consumption.rrd:km:AVERAGE", 
        "DEF:con_h=consumption.rrd:h:AVERAGE", 
        "AREA:con_km#f00000ee:l/100km", 
        "LINE2:con_h#aaaaaaee:l/h",
        NULL    // array needs to be terminated with NULL
    };
        
    // append args to combined
    for (i = 0; args_graph[i] != NULL; i++) 
        strncpy(combined[i], args_graph[i], sizeof(combined[i]));
    
    // append rrdstyle to combined
    for (n = 0; rrdstyle[n] != NULL; n++, i++) 
        strncpy(combined[i], rrdstyle[n], sizeof(combined[i]));
    
    // append xgrid setting
    strncpy(combined[i], "--x-grid", sizeof(combined[i]));
    i++;
    
    if (consumption_graph_timespan < 900)        // 15min
        strncpy(combined[i], xgrid_short, sizeof(combined[i]));
    else if (consumption_graph_timespan < 3600)  // 1h
        strncpy(combined[i], xgrid_medium, sizeof(combined[i]));
    else
        strncpy(combined[i], xgrid_long, sizeof(combined[i]));
    
    i++;
    
    // we need an array of pointers for execvp()
    for (n = 0; n < i; n++)
        args_graph[n] = combined[n];
    
    
    for (i = 0; args_graph[i] != NULL; i++);

    if (rrd_graph_v(i, args_graph) == NULL)
        perror("rrd_graph_v() error");
    
    return;
}


void
rrdtool_update_speed (void)
{
    char    starttime[256];
    char    endtime[256];
    time_t  t;
    int     i, n;

    char    combined[256][256];
    
    float   speed;
   
    speed = get_value("speed");
 
    if (speed >= 0)
    { 
        snprintf(starttime, sizeof (starttime), "%d:%.1f", 
                 (int) time (&t), speed);
        
        // rrdtool args
        char *args_update[256] = 
        {
            "update", "speed.rrd", starttime,
            NULL
        };
        
        for (i = 0; args_update[i] != NULL; i++);

        if (rrd_update(i, args_update) == -1)
            perror("rrd_update() error");
    }
    
    
    // we want to graph the last 5 mins
    snprintf(starttime, sizeof(starttime), "%d",
             (int) time (&t) - speed_graph_timespan); // now - timespan
    
    snprintf(endtime, sizeof(endtime), "%d",
             (int) time (&t));
    
    
    char *args_graph[256] =
    { 
        "graph", SPEED_GRAPH,
        "--start", starttime, "--end", endtime,
        "--upper-limit", "135", "--lower-limit", "0", "--y-grid", "7.5:2",
        "DEF:myspeed=speed.rrd:speed:AVERAGE",
        "AREA:myspeed#f00000ee:km/h", 
        NULL    // array needs to be terminated with NULL
    };
    
    // append args to combined
    for (i = 0; args_graph[i] != NULL; i++) 
        strncpy(combined[i], args_graph[i], sizeof(combined[i]));
    
    // append rrdstyle to combined
    for (n = 0; rrdstyle[n] != NULL; n++, i++) 
        strncpy(combined[i], rrdstyle[n], sizeof(combined[i]));
    
    // append xgrid setting
    strncpy(combined[i], "--x-grid", sizeof(combined[i]));
    i++;
    
    if (speed_graph_timespan < 900)        // 15min
        strncpy(combined[i], xgrid_short, sizeof(combined[i]));
    else if (speed_graph_timespan < 3600)  // 1h
        strncpy(combined[i], xgrid_medium, sizeof(combined[i]));
    else
        strncpy(combined[i], xgrid_long, sizeof(combined[i]));
    
    i++;
    
    // we need an array of pointers for execvp()
    for (n = 0; n < i; n++)
        args_graph[n] = combined[n];
    
    
    for (i = 0; args_graph[i] != NULL; i++);

    if (rrd_graph_v(i, args_graph) == NULL)
        perror("rrd_graph_v() error");
    
    return;
}

void
rrdtool_create_consumption (void)
{
    time_t  t;
    FILE   *fp;
    
    int     i;
    char    starttime[256];
    
    
    fp = fopen ("consumption.rrd", "rw");
    if (fp)
    {
        fclose (fp);
        return;
    }
    
    printf ("creating consumption rrd database\n");

    snprintf (starttime, sizeof (starttime), "%d", (int) time (&t));
        
    char *args[256] = 
    {
        "create", "consumption.rrd",
        "--start", starttime, "--step", "1",
        "DS:km:GAUGE:30:U:U",                   // unknown after 30sec
        "DS:h:GAUGE:5:0:U",                     // unknown after 5sec
        "RRA:AVERAGE:0.5:1:300",                // 5min 
        "RRA:AVERAGE:0.5:5:360",                // 30min
        "RRA:AVERAGE:0.5:40:360",               // 4h
        NULL
    };
    
    
    for (i = 0; args[i] != NULL; i++);
	if (rrd_create(i, args) == -1)
            perror("rrd_create() error");
    
    return;
}


void
rrdtool_create_speed (void)
{
    time_t  t;
    FILE   *fp;
    
    char    starttime[256];
    int     i;
    
    
    fp = fopen ("speed.rrd", "rw");
    if (fp)
    {
        fclose (fp);
        return;
    }
    
    printf ("creating speed rrd database\n");
    
    snprintf (starttime, sizeof (starttime), "%d", (int) time (&t));
    
    // rrdtool create
    char *args[256] = 
        {
            "create", "speed.rrd",
            "--start", starttime, "--step", "1",
            "DS:speed:GAUGE:30:0:200",              // unknown after 30sec
            "RRA:AVERAGE:0.5:1:300",                // 5min 
            "RRA:AVERAGE:0.5:5:360",                // 30min
            "RRA:AVERAGE:0.5:40:360",               // 4h
            NULL
        };
        
    
    for (i = 0; args[i] != NULL; i++);
	if (rrd_create(i, args) == -1)
            perror("rrd_create() error");

    return;
}
