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
 * bigger font size for average values below graphs (done)
 *
 * kw1281_open() in for (;;)
 *
 * migrate /tmp files back to home (disable symlinks) (done)
 *
 * high priority / nice only for kw1281 process
 *
 * suddenly high error rate and only works with DEBUG on. WTF?
 *   only change -> timespan->average struct and added gval liters
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
    int     fd;
    key_t   key = 51337;

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
    gval->liters    =  0;
    
    // init average structs
    av_con->array_full = 0;
    av_con->counter = 0;
    av_con->average_short = 0; 
    av_con->average_medium = 0;
    av_con->average_long = 0;
    av_con->timespan = 300;    
    
    av_speed->array_full = 0;
    av_speed->counter = 0;
    av_speed->average_short = 0; 
    av_speed->average_medium = 0;
    av_speed->average_long = 0;
    av_speed->timespan = 300;
    
    
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
