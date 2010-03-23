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
 * fix liters calculation with measuring time between calls
 *
 * fix average calculations:
 *   - measure time
 *   - better handling of 0km / 0l/100km handling
 *
 * set baudrate, multiplicator value and other things via config file
 * set baudrate from argv
 *
 * find a way to get tank content on user request (button)
 *
 */

int     init_values(void);
void	reset_values(void);
void	cleanup (int);

// shmid has to be global, so we can destroy it on exit
int     shmid;
void   *p;

// pid of ajax server
pid_t   pid;

// flag for cleanup function
char	cleaning_up = 0;


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
    
    reset_values();   
 
    // init average structs
    av_con->array_full = 0;
    av_con->counter = 0;
    av_con->average_short = 0; 
    av_con->average_medium = 0;
    av_con->average_long = 0;
    //av_con->liters = 0;
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


void
reset_values(void)
{
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
    gval->tank      = -2;
    
}

void
cleanup (int signo)
{
	// if we're already cleaning up, do nothing
	if (cleaning_up)
		return;
	
	cleaning_up = 1;
	
	printf("\ncleaning up:\n");
	
#ifdef SERIAL_ATTACHED		
	printf("closing serial port...\n");
	kw1281_close();
#endif
	
	// send kill to ajax server 
	printf("sending SIGTERM to ajax server...\n");
	kill(pid, SIGTERM);
	
	printf("freeing shm memory...\n");
	// free shm memory segment
	if (shmdt(p) == -1)
		perror("shmdt()");
	else if (shmctl(shmid, IPC_RMID, NULL) == -1)
		perror("shmctl()");
	
	printf("exiting.\n");
	exit(0);
}


int
main (int arc, char **argv)
{
    //int     status;
    int     ret;
    
    
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
    
	
	// add signal handler for cleanup function
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);	
	
    
    // set realtime priority if we're running as root
#ifdef HIGH_PRIORITY
	struct sched_param prio;
	
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
    
    
    /* get the content of the tank 
     * this is done only once, because we would've
     * to disconnect ecu and reconnect to instruments
     * every time.. 
     */
#ifdef SERIAL_ATTACHED        
    // ECU: 0x01, INSTR: 0x17
    // send 5baud address, read sync byte + key word
    ret = kw1281_init (0x17);
    
    // soft error, e.g. communication error
    if (ret == -1)
        ajax_log("init for instruments failed, skipping...\n");
    
    // hard error (e.g. serial cable unplugged)
    else if (ret == -2)
    {
        ajax_log("serial port error\n");
        cleanup(0);
    }
    
#endif
    
    if (kw1281_get_tank_cont() == -1)
        ajax_log("error getting tank content, skipping...\n");
    
    
    for ( ; ; )
    {
        
#ifdef SERIAL_ATTACHED
        printf ("init\n");                
        
        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        ret = kw1281_init (0x01);
        
        // soft error, e.g. communication error
        if (ret == -1)
        {
            ajax_log("init failed, retrying...\n");
            reset_values();
            continue;
        }
        
        // hard error (e.g. serial cable unplugged)
        else if (ret == -2)
		{
			ajax_log("serial port error\n");
            cleanup(0);
		}
#endif

        if (kw1281_mainloop() == -1)
        {
            ajax_log("errors. restarting...\n");
            reset_values();
            continue;
        }
    }

    // should never be reached
    return 0;
}
