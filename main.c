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
 * check if "hang on socket shutdown" error is fixed
 *
 * kw1281_open() in for (;;)
 *
 * high priority / nice only for kw1281 process
 *
 * cleanup function, catch sigint, shutdown ajax server
 *
 * fix liters calculation with measuring time between calls
 *
 * only works as long as writing to file is disabled. WTF
 *
 * set baudrate, multiplicator value and other things via config file
 *
 */

int     init_values(void);

// shmid has to be global, so we can destroy it on exit
int     shmid;
void   *p;


int
init_values(void)
{
    int     file;
    key_t   key = 13337;

    // setup shared values
    if ( (shmid = shmget(key,
                         sizeof (struct values) +
                         2 * sizeof(struct average) +
                         1024, // sizeof debug 
                         0666 | IPC_CREAT)) < 0)
    {
        perror("shmget()");
        return -1;
    }
    
    if ( (p = shmat(shmid, NULL, 0)) == (void *) -1)
    {
        perror("shmat()");
        return -1;
    }
    
    // attach variables to shared memory space
    gval     = (struct values *) p;
    av_speed = (struct average *) (p + sizeof(struct values));
    av_con   = (struct average *) (p + sizeof(struct values) + sizeof(struct average));
    debug    = (char *) (p + sizeof(struct values) + 2 * sizeof(struct average));    
    
    
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
    
    // init average structs
    av_con->array_full = 0;
    av_con->counter = 0;
    av_con->average_short = 0; 
    av_con->average_medium = 0;
    av_con->average_long = 0;
    av_con->liters = 0;
    av_con->timespan = 300;    
    
    av_speed->array_full = 0;
    av_speed->counter = 0;
    av_speed->average_short = 0; 
    av_speed->average_medium = 0;
    av_speed->average_long = 0;
    av_speed->timespan = 300;
  

    // use & 
    // overwrite consumption inits from file, if present
    if ( (file = open( CON_AV_FILE, O_RDONLY )) != -1)
    {
        read(file, av_con, sizeof(struct average));
        close( file );
    }
    
    // overwrite speed inits from file, if present
    if ( (file = open( SPEED_AV_FILE, O_RDONLY )) != -1)
    {
        read(file, av_speed, sizeof(struct average));
        close( file );
    }
    
    
#ifdef DEBUG  
    int i;
    printf("av_con->counter: %d\n", av_con->counter);
    for (i = 0; i < av_con->counter; i++)
        printf("%.02f ", av_con->array[i]);
    printf("array full? (%d)\n", av_con->array_full);
    printf("av_con averages: %.02f, %.02f, %.02f\n",
           av_con->average_short, av_con->average_medium, av_con->average_long);

    printf("av_speed->counter: %d\n", av_speed->counter);
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
    //int     status;
    int     ret;
    struct sched_param prio;
    
    
#ifdef SERIAL_ATTACHED
    // kw1281_open() somehow has to be started
    // before any fork() open()
    if (kw1281_open (DEVICE) == -1)
        return -1;
#endif
    
    // initialize values (if possible, load from file)
    if (init_values() == -1)
        return -1;
    
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
    
    
    for ( ; ; )
    {
#ifdef SERIAL_ATTACHED
        printf ("init\n");                
        
        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        ret = kw1281_init (0x01);
        
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
            
            /*
             * we can't call ajax_shutdown() directly
             * because its running in another process
             */
            // ajax_shutdown();
            // waitpid(pid, &status, 0);
            
            if (shmdt(p) == -1)
                perror("shmdt()");
            else if (shmctl(shmid, IPC_RMID, NULL) == -1)
                perror("shmctl()");
            
            return -1;
        }
#endif

        if (kw1281_mainloop() == -1)
        {
            printf("errors. restarting...\n");

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
        }
        
    }

    // should never be reached
    return 0;
}
