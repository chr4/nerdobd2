#include "core.h"
#include "../common/config.h"

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

    // for testing purposes
    int i = 0, flag = 0;
    for (; ;)
    {
        handle_data("rpm", 1000 + i * 100);
        handle_data("injection_time", 0.15 * i);
        handle_data("oil_pressure", i);
        handle_data("speed", 3 * i );
        handle_data("temp_engine", 90);
        handle_data("temp_air_intake", 35);
        handle_data("voltage", 0.01 * i);
        usleep(300000);

        if (i > 35)
            flag = 1;
        if (i < 2)
            flag = 0;

        if (flag)
            i--;
        else
            i++;
    }


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


// this struct collects all engine data
// before sending it to database
engine_data engine;

void
handle_data(char *name, float value)
{
    /* first block gets rpm, injection time, oil pressure
     * second block gets speed
     * third block gehts voltage and temperatures (not as often requested)
     */

    if (!strcmp(name, "rpm"))
        engine.rpm = value;
    else if (!strcmp(name, "injection_time"))
        engine.injection_time = value;
    else if (!strcmp(name, "oil_pressure"))
        engine.oil_pressure = value;

    // everytime we get speed, calculate consumption
    // end send data to database
    else if (!strcmp(name, "speed"))
    {
        engine.speed = value;

        // calculate consumption per hour
        engine.per_h = 60 * 4 * MULTIPLIER * engine.rpm * (engine.injection_time - INJ_SUBTRACT);

        // calculate consumption per hour
        if ( engine.speed > 0)
            engine.per_km = engine.per_h / engine.speed * 100;
        else
            engine.per_km = -1;


        db_send_engine_data(engine);
    }

    // put other values directly to database
    else 
        db_send_other_data(name, value);
}
