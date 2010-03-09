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
 * create nice looking interface
 *  -> fonts?
 *
 * fix communication errors with ECU, resp recover on errors
 *
 * use shm instead of pthread shared variables (why?)
 *
 *
 * howto properly close serial port
 * (so we don't have timeout problems on reconnect)
 *
 *
 * fastinit is unused atm (and not working)
 * recover is unused atm (and not working)
 *
 */

void    init_values(void);


void
init_values(void)
{
    int fd;
    
    /* init values with -2, so ajax socket can control if data is availiable */
    speed = -2;
    rpm = -2;
    temp1 = -2;
    temp2 = -2;
    oil_press = -2;
    inj_time = -2;
    voltage = -2;
    con_h = -2;
    con_km = -2;
    
    
    // init average structs
    av_con.array_full = 0;
    av_con.counter = 0;
    av_con.average_short = 0; 
    av_con.average_medium = 0;
    av_con.average_long = 0;
    
    av_speed.array_full = 0;
    av_speed.counter = 0;
    av_speed.average_short = 0; 
    av_speed.average_medium = 0;
    av_speed.average_long = 0;
    
    
    // overwrite consumption inits from file, if present
    if ( (fd = open( CON_AV_FILE, O_RDONLY )) != -1)
    {
        read(fd, &av_con, sizeof(av_con));
        close( fd );
    }
    
    // overwrite speed inits from file, if present
    if ( (fd = open( SPEED_AV_FILE, O_RDONLY )) != -1)
    {
        read(fd, &av_speed, sizeof(av_speed));
        close( fd );
    }
    
#ifdef DEBUG  
    int i;
    printf("av_con.counter: %d\n", av_con.counter);
    for (i = 0; i < av_con.counter; i++)
        printf("%.02f ", av_con.array[i]);
    printf("array full? (%d)\n", av_con.array_full);
    printf("av_con averages: %.02f, %.02f, %.02f\n",
           av_con.average_short, av_con.average_medium, av_con.average_long);

    printf("av_speed.counter: %d\n", consumption.counter);
    for (i = 0; i < av_speed.counter; i++)
        printf("%.02f ", av_speed.array[i]);
    printf("array full? (%d)\n", av_speed.array_full);
    printf("speed averages: %.02f, %.02f, %.02f\n",
           av_speed.average_short, av_speed.average_medium, av_speed.average_long);    
#endif  
}

int
main (int arc, char **argv)
{
    pthread_t thread1;
    struct sched_param prio;
    
    // set realtime priority if we're running as root
    if (getuid() == 0)
    {
        prio.sched_priority = 1;
        
        if ( sched_setscheduler(getpid(), SCHED_FIFO, &prio) < 0)
        {
            perror("sched_setscheduler");
        }
    }
    else
        printf("sorry, need to be root for realtime priority. continuing with normal priority.\n");
    
    
#ifdef SERIAL_ATTACHED
    // kw1281_open() somehow has to be started
    // before any threading stuff.
    if (kw1281_open (DEVICE) == -1)
       return -1;
#endif

    // create databases, unless they exist
    rrdtool_create_consumption ();
    rrdtool_create_speed ();

    // initialize values (if possible, load from file)
    init_values();

    
    // create ajax socket in new thread for handling http connections
    pthread_create (&thread1, NULL, ajax_socket, (void *) PORT);

    /* this loop is intended to restart the connection
     * on connection errors, unfortunately, somehow kw1281_open()
     * only works before any threading and forking
     */
    // for ( ; ; )
    // {
#ifdef SERIAL_ATTACHED
        printf ("init\n");                
        
        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        if (kw1281_init (0x01) == -1)
        //if (kw1281_fastinit (0x01) == -1)
        {
            printf("init failed. exiting...\n");
            kw1281_close();
            return -1;
        }
#endif

        if (kw1281_mainloop() == -1)
        {
            printf("errors. exiting...\n");
            kw1281_close();
            ajax_shutdown();
            pthread_kill(thread1, SIGTERM);
            pthread_join(thread1, NULL);
            
            return -1;
        }
        
    // }

    // should never be reached
    kw1281_close();    
    return 0;
}
