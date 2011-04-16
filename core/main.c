#include "core.h"

void	cleanup (int);

// flag for cleanup function
char	cleaning_up = 0;

void
cleanup (int signo)
{
	// if we're already cleaning up, do nothing
	if (cleaning_up)
		return;
	
	cleaning_up = 1;
	
	printf("\ncleaning up:\n");
	
	printf("closing serial port...\n");
	kw1281_close();
	
	printf("exiting.\n");
	exit(0);
}


int
main (int argc, char **argv)
{
    int     ret;

    // kw1281_open() somehow has to be started
    // before any fork() open()
    if (kw1281_open (DEVICE) == -1)
        return -1;

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
    
    for ( ; ; )
    {
        
        printf ("init\n");                
        
        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        ret = kw1281_init (0x01);
        
        // soft error, e.g. communication error
        if (ret == -1)
        {
            printf("init failed, retrying...\n");
            continue;
        }
        
        // hard error (e.g. serial cable unplugged)
        else if (ret == -2)
        {
            printf("serial port error\n");
            cleanup(0);
        }

        
        ret = kw1281_mainloop();
        
        // on errors, restart
        if (ret == -1)
        {
            printf("errors. restarting...\n");
            continue;
        }
        
    }

    // should never be reached
    return 0;
}
