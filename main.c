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
 */

void	reset_values(void);
void	cleanup (int);
void    refresh_tank_content(void);

// pid of ajax server
pid_t   pid;

// flag for cleanup function
char	cleaning_up = 0;


void
reset_values(void)
{
    return;
}

// refreshes tank content
void
refresh_tank_content(void)
{
    int     trys;

#ifdef SERIAL_ATTACHED
    int     ret;
#endif

    // try up to TANK_CONT_MAX_TRYS times
    for (trys = 0; trys < TANK_CONT_MAX_TRYS; trys++)
    { 
        
#ifdef SERIAL_ATTACHED        
        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        ret = kw1281_init (0x17);
    
        // soft error, e.g. communication error
        if (ret == -1) {
            ajax_log("init (tank content) failed, retrying...\n");
            continue;
        }
    
        // hard error (e.g. serial cable unplugged)
        else if (ret == -2)
        {
            ajax_log("serial port error\n");
            cleanup(0);
        }
    
#endif
    
        if (kw1281_get_tank_cont() == -1)
        {
            ajax_log("error getting tank content, retrying...\n");
            continue;
        }
        // on success, break
        else
        {
            ajax_log("tank content updated.\n");
            break;
        }
    }
    
    // reset flag
    tank_request = 0;
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
    if (init_db() == -1)
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
    
    
    // getting tank content once before entering main loop
    // refresh_tank_content();
    
    
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

        
        ret = kw1281_mainloop();
        
        // on errors, restart
        if (ret == -1)
        {
            ajax_log("errors. restarting...\n");
            reset_values();
            continue;
        }
        
        // if mainloop() exited due to tank request
        else if (ret == TANK_REQUEST)
        {
            // grey out values
            reset_values();
            refresh_tank_content();
        }
    }

    // should never be reached
    return 0;
}
