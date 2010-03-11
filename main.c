#include "serial.h"

/*
 * start like this:
 * # while (true); do ./serial; done
 *
 * thanks: Florian Schäffer     http://www.blafusel.de/obd/obd2_kw1281.html
 *         Philipp aka pH5      http://www.blafusel.de/phpbb/viewtopic.php?t=223
 *         freediag             http://freediag.org/
 *
 *
 * TODO:
 * 
 * resetting counters works, but unlike the speed averages
 * the consumption averages get updated on first new value
 * instead of directly being set to 0 (wtf? code is the same.. )
 *
 *
 * create hover tooltips for switching graph timespans
 *
 * set baudrate, multiplicator value and other things via config file
 *
 */

int     init_values(void);

// shmid has to be global, so we can destroy it on exit
int     shmid_gval;
int     shmid_speed;
int     shmid_con;
int     shmid_debug;

int
init_values(void)
{
    int     fd;
    key_t   key1 = 1337;
    key_t   key2 = 31337;
    key_t   key3 = 31338;
    key_t   key4 = 1338;

    // setup shared values
    if ( (shmid_gval = shmget(key1, sizeof(struct values), 0666 | IPC_CREAT)) < 0)
    {
        perror("shmget()");
        return -1;
    }
    
    if ( (gval = (struct values *) shmat(shmid_gval, (void *) 0, 0)) == (void *) -1)
    {
        perror("shmat()");
        return -1;
    }
    
    // setup average speed shared values
    if ( (shmid_speed = shmget(key2, sizeof(struct average), 0666 | IPC_CREAT)) < 0)
    {
        perror("shmget()");
        return -1;
    }
    
    if ( (av_speed = (struct average *) shmat(shmid_speed, (void *) 0, 0)) == (void *) -1)
    {
        perror("shmat()");
        return -1;
    }
    
    // setup average consumption shared values
    if ( (shmid_con = shmget(key3, sizeof(struct average), 0666 | IPC_CREAT)) < 0)
    {
        perror("shmget()");
        return -1;
    }
    
    if ( (av_con = (struct average *) shmat(shmid_con, (void *) 0, 0)) == (void *) -1)
    {
        perror("shmat()");
        return -1;
    }
    
    // setup debugging string
    if ( (shmid_debug = shmget(key4, 1024, 0666 | IPC_CREAT)) < 0)
    {
        perror("shmget()");
        return -1;
    }
    
    if ( (debug = (char *) shmat(shmid_debug, (void *) 0, 0)) == (void *) -1)
    {
        perror("shmat()");
        return -1;
    }
    
    
    /* init values with -2
     * so ajax socket can control 
     * if data is availiable 
     */
    gval->speed     = -2;
    gval->rpm       = -2;
    gval->temp1     = -2;
    gval->temp2     = -2;
    gval->oil_press = -2;
    gval->inj_time  = -2;
    gval->voltage   = -2;
    gval->con_h     = -2;
    gval->con_km    = -2;
    
    // default timespan 5min (300sec)
    gval->con_timespan = 300;    
    gval->speed_timespan = 300;
    
    // init average structs
    av_con->array_full = 0;
    av_con->counter = 0;
    av_con->average_short = 0; 
    av_con->average_medium = 0;
    av_con->average_long = 0;
    
    av_speed->array_full = 0;
    av_speed->counter = 0;
    av_speed->average_short = 0; 
    av_speed->average_medium = 0;
    av_speed->average_long = 0;
    
    
    // overwrite consumption inits from file, if present
    if ( (fd = open( CON_AV_FILE, O_RDONLY )) != -1)
    {
        read(fd, av_con, sizeof(struct average));
        close( fd );
    }
    
    // overwrite speed inits from file, if present
    if ( (fd = open( SPEED_AV_FILE, O_RDONLY )) != -1)
    {
        read(fd, av_speed, sizeof(struct average));
        close( fd );
    }
    
#ifdef DEBUG  
    int i;
    printf("av_con->counter: %d\n", av_con->counter);
    for (i = 0; i < av_con->counter; i++)
        printf("%.02f ", av_con->array[i]);
    printf("array full? (%d)\n", av_con->array_full);
    printf("av_con averages: %.02f, %.02f, %.02f\n",
           av_con->average_short, av_con->average_medium, av_con->average_long);

    printf("av_speed->counter: %d\n", consumption.counter);
    for (i = 0; i < av_speed->counter; i++)
        printf("%.02f ", av_speed->array[i]);
    printf("array full? (%d)\n", av_speed->array_full);
    printf("speed averages: %.02f, %.02f, %.02f\n",
           av_speed->average_short, av_speed->average_medium, av_speed->average_long);    
#endif  
    
    return 0;
}



int
main (int arc, char **argv)
{
    pid_t   pid;
    int     status;
    int     ret;
    struct sched_param prio;
    
    
    // initialize values (if possible, load from file)
    if (init_values() == -1)
        return -1;
    
    
#ifdef SERIAL_ATTACHED
    // kw1281_open() somehow has to be started
    // before any threading stuff.
    if (kw1281_open (DEVICE) == -1)
       return -1;
#endif
    
    // link speed.png and consumption.png into .
    symlink(SPEED_GRAPH, "speed.png");
    symlink(CON_GRAPH, "consumption.png");
    
    // create databases, unless they exist
    rrdtool_create_consumption ();
    rrdtool_create_speed ();

    
    // create ajax socket in child process for handling http connections
    if ( (pid = fork()) > 0)
    {
        ajax_socket(PORT);
        exit(0);
    }
    
    
    // set realtime priority if we're running as root
#ifdef HIGH_PRIORITY
    if (getuid() == 0)
    {
        prio.sched_priority = 1;
        
        if ( sched_setscheduler(getpid(), SCHED_FIFO, &prio) < 0)
            perror("sched_setscheduler");
        
        if (nice(-19) == -1)
            perror("nice failed\n");
    }
    else
        printf("sorry, need to be root for realtime priority. continuing with normal priority.\n");
#endif
    
    /* this loop is intended to restart the connection
     * on connection errors, unfortunately, somehow kw1281_open()
     * only works before any threading and forking, so its of 
     * no use at the moment
     */
    
    for ( ; ; )
    {
#ifdef SERIAL_ATTACHED
        printf ("init\n");                
        
        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        ret = kw1281_init (0x01);
        // ret = kw1281_fastinit (0x01);
        
        if (ret == -1)
        {
            printf("init failed, retrying...\n");
            
            /* set values to -2 so ajax interfaces
             * knows we're not having new values
             * for a short while and can grey them out
             */
            gval->speed     = -2;
            gval->rpm       = -2;
            gval->temp1     = -2;
            gval->temp2     = -2;
            gval->oil_press = -2;
            gval->inj_time  = -2;
            gval->voltage   = -2;
            gval->con_h     = -2;
            gval->con_km    = -2;
            
            continue;
        }
        
        else if (ret == -2)
        {
            printf("serial port error, exiting.\n");
            kw1281_close();
            ajax_shutdown();
            
            waitpid(pid, &status, 0);
            
            if (shmdt(gval) == -1)
                perror("shmdt()");
            else if (shmctl(shmid_gval, IPC_RMID, NULL) == -1)
                perror("shmctl()");
            
            if (shmdt(av_speed) == -1)
                perror("shmdt()");
            else if (shmctl(shmid_speed, IPC_RMID, NULL) == -1)
                perror("shmctl()");
            
            if (shmdt(av_con) == -1)
                perror("shmdt()");
            else if (shmctl(shmid_con, IPC_RMID, NULL) == -1)
                perror("shmctl()");
            
            if (shmdt(debug) == -1)
                perror("shmdt()");
            else if (shmctl(shmid_debug, IPC_RMID, NULL) == -1)
                perror("shmctl()");
            
            return -1;
        }
#endif

        if (kw1281_mainloop() == -1)
            printf("errors. restarting...\n");
        
    }

    // should never be reached
    return 0;
}
