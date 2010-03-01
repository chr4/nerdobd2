#include "serial.h"

/*
 * TODO:
 *
 * create timer for sensing heartbeat
 * kill on problems
 *
 */

int
main (int arc, char **argv)
{
    pthread_t thread1;

#ifdef SERIAL_ATTACHED
    // kw1281_open() somehow has to be started
    // before any threading stuff.
    if (kw1281_open (DEVICE) == -1)
       return -1;
#endif

    // create databases, unless they exist
    rrdtool_create_consumption ();
    rrdtool_create_speed ();

    // create ajax socket in new thread for handling http connections
    pthread_create (&thread1, NULL, ajax_socket, (void *) PORT);

    for ( ; ; )
    {
#ifdef SERIAL_ATTACHED
        printf ("init\n");                
        
        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        if (kw1281_init (0x01) == -1)
        {
            printf("init failed. trying again...\n");
            continue;
        }
#endif

        if (kw1281_mainloop() == -1)
        {
            printf("errors. exiting...\n");
            pthread_kill(thread1, SIGTERM);
            pthread_join(thread1, NULL);
            return -1;
        }
        
    }
    
    return 0;
}
